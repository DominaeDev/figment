#pragma once

#include "Figment.h"
#include "io/Error.h"
#include "io/XmlData.h"
#include "io/AsyncDownloader.h"
#include "data/VersionNumber.h"

namespace fig::data
{
	enum TargetPlatform
	{
		Undefined = 0,
		Windows,
	};

	constexpr auto TargetPlatformMapping = std::array<std::pair<TargetPlatform, std::string_view>, 2uz> {
		std::pair { TargetPlatform::Undefined,	"Undefined" },
		std::pair { TargetPlatform::Windows,	"Windows" },
	};

	enum class PackageType
	{
		Undefined = 0,
		LLM_Model,
		TTS_Model,
		TTS_Server,
	};

	constexpr auto PackageTypeMapping = std::array<std::pair<PackageType, std::string_view>, 4uz> {
		std::pair { PackageType::Undefined,		"Undefined" },
		std::pair { PackageType::LLM_Model,		"LLM_Model" },
		std::pair { PackageType::TTS_Model,		"TTS_Merver" },
		std::pair { PackageType::TTS_Server,	"TTS_Server" },
	};

	struct PackageInfo
	{
		fig::uuid id;
		PackageType type {};
		TargetPlatform targetPlatform {};
		VersionNumber version;
		uint64_t fileSize;
		fig::string name;
		fig::string description;
		fig::string downloadUrl;
		fig::string infoUrl;
		fig::string sha256;
		fig::path targetPath;

		struct FileEntry
		{
			fig::uuid id;
			uint64_t fileSize;
			fig::string name;
			fig::string sha256;
			fig::path targetPath;

			static auto XmlFields() noexcept
			{
				return Fields(
					Attribute("id", &FileEntry::id)
						.MustExist(),
					Element("Name", &FileEntry::name)
						.MustExist(),
					Element("Target", &FileEntry::targetPath)
						.MustExist(),
					Element("Sha256", &FileEntry::sha256),
					Element("Size", &FileEntry::fileSize)
				);

				static_assert(IsXmlSerializable<FileEntry>);
			}
		};
		std::vector<FileEntry> entries; // Archive

		static auto XmlFields() noexcept
		{
			return Fields(
				Attribute("id", &PackageInfo::id)
					.MustExist(),
				Element("Type", &PackageInfo::type,
					[](auto&& value) { return enum_serialize(value, PackageTypeMapping); },
					[](auto&& value) { return enum_deserialize(value, PackageTypeMapping); })
					.MustExist(),
				Element("Platform", &PackageInfo::targetPlatform,
					[](auto&& value) { return enum_serialize(value, TargetPlatformMapping); },
					[](auto&& value) { return enum_deserialize(value, TargetPlatformMapping); }),
				Element("Name", &PackageInfo::name)
					.MustExist(),
				Element("Description", &PackageInfo::description),
				Element("DownloadUrl", &PackageInfo::downloadUrl)
					.MustExist(),
				Element("InfoUrl", &PackageInfo::infoUrl),
				Element("Size", &PackageInfo::fileSize)
					.MustExist(),
				Element("Sha256", &PackageInfo::sha256),
				Element("Version", &PackageInfo::version),
				Element("File", &PackageInfo::entries)
					.Collection("Archive")
			);
			static_assert(IsXmlSerializable<PackageInfo>);
		}
	};
}

namespace fig::io
{
	class AsyncDownloader;

	enum class PackageState
	{
		Unknown = -1,
		NotDownloaded = 0,
		PartiallyDownloaded,
		Unverified,
		Installed,
		VerificationFailed,
	};

	enum class InstallationPhase
	{
		None = 0,
		Downloading,
		Decompressing,
		Verifying,
		Installing,
		Completed,
		Failed,
	};

	struct InstallationState
	{
		InstallationPhase phase {};
		uint64_t bytesReceived {};
		uint64_t bytesTotal {};

		float GetProgress() const noexcept
		{
			if (bytesTotal == 0)
				return 0.0f;

			return static_cast<float>(static_cast<double>(bytesReceived) / static_cast<double>(bytesTotal));
		}
	};

	class PackageManager : public fig::data::XmlData<"Packages", 0>
	{
	public:
		PackageManager();
		~PackageManager();

		FileError Init() noexcept;
		void CheckInstalledPackages();

		fig::optional_cref<fig::data::PackageInfo> GetPackage(const fig::uuid&) const noexcept;
		const std::vector<fig::data::PackageInfo>& GetPackages() const noexcept;

		bool InstallPackage(const fig::uuid& packageId);
		bool CancelInstall(fig::uuid packageId);
		void CancelAll();

		PackageState GetPackageState(const fig::uuid& packageId) const;
		InstallationState GetInstallationState(const fig::uuid& packageId) const;

	private:
		FileError LoadState();
		FileError SaveState();

		std::vector<fig::data::PackageInfo> _packages;
		std::unique_ptr<std::jthread> _verificationWorker {};
		std::map<fig::uuid, PackageState> _packageStates;
		std::map<fig::uuid, fig::string> _packageHashes;
		
		void __CheckInstalledPackages();
		struct Installation
		{
			fig::data::PackageInfo packageInfo;
			std::unique_ptr<Downloader> downloader;
			std::unique_ptr<std::jthread> thread;

			enum class Error
			{
				NoError = 0,
				Cancelled,
				DownloadError,
				FileError,
				VerificationFailed,
			} error;

			DownloadError downloadError {};
			FileError fileError {};
			InstallationPhase phase;
		};
		std::map<fig::uuid, Installation> _activeInstalls;
		std::unordered_set<fig::uuid> _finishedInstalls;
		void __InstallPackage(fig::uuid packageId, std::stop_token stopToken);

		mutable std::mutex _mutex; // Guards all state

	public:
		static auto XmlFields() noexcept
		{
			using namespace fig::data;
			return Fields(
				Element("Package", &PackageManager::_packages)
					.MustExist()
			);

			static_assert(IsXmlSerializable<PackageManager>);
		}
	};
}
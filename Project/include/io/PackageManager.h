#pragma once

#include "Figment.h"
#include "io/Error.h"
#include "io/XmlData.h"
#include "io/AsyncDownloader.h"
#include "data/VersionNumber.h"
#include "text/Context.h"

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
		LLMModel,
		TTSServer,
		TTSVoiceModel,
		TTSDesignModel,
	};

	constexpr auto PackageTypeMapping = std::array<std::pair<PackageType, std::string_view>, 5uz> {
		std::pair { PackageType::Undefined,			"undefined" },
		std::pair { PackageType::LLMModel,			"llm_model" },
		std::pair { PackageType::TTSServer,			"tts_server" },
		std::pair { PackageType::TTSVoiceModel,		"tts_model" },
		std::pair { PackageType::TTSDesignModel,	"tts_design_model" },
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
		fig::string versionString;
		fig::string dependencies;
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
				Attribute("type", &PackageInfo::type,
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
				Element("VersionString", &PackageInfo::versionString),
				Element("Dependencies", &PackageInfo::dependencies),
				Element("Target", &PackageInfo::targetPath),
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

	struct InstallationProgress
	{
		InstallationPhase phase {};
		uint64_t bytesReceived {};
		uint64_t bytesTotal {};
		fig::string errorMessage {};

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

		std::pair<fig::optional_cref<fig::data::PackageInfo>, PackageState> GetPackage(const fig::uuid&) const noexcept;
		const std::vector<fig::data::PackageInfo>& GetPackages() const noexcept;
		std::vector<fig::data::PackageInfo> GetInstalledPackages() const noexcept;

		bool InstallPackage(const fig::uuid& packageId);
		bool UninstallPackage(const fig::uuid& packageId);
		bool CancelInstall(fig::uuid packageId);
		void CancelAll();

		PackageState GetPackageState(const fig::uuid& packageId) const;
		InstallationProgress GetInstallationProgress(const fig::uuid& packageId) const;

		fig::Context GetContext() const;

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
			InstallationPhase phase;

			enum class Error
			{
				NoError = 0,
				Cancelled,
				DownloadFailed,
				VerificationFailed,
			};

			using Result = std::variant<Error, FileError>;
			Result result {};
		};
		std::map<fig::uuid, Installation> _activeInstalls;
		std::unordered_set<fig::uuid> _finishedInstalls;
		bool _bChanged {};
		void __InstallPackage(fig::uuid packageId, std::stop_token stopToken);
		void __CleanUpTemporaryFiles(fig::uuid packageId);

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
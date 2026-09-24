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
		std::pair { TargetPlatform::Undefined,	"undefined" },
		std::pair { TargetPlatform::Windows,	"windows" },
	};

	enum class PackageType
	{
		Undefined = 0,
		LLM_Model,
		TTS_Model,
		TTS_Server,
	};

	constexpr auto PackageTypeMapping = std::array<std::pair<PackageType, std::string_view>, 4uz> {
		std::pair { PackageType::Undefined,		"undefined" },
		std::pair { PackageType::LLM_Model,		"llm_model" },
		std::pair { PackageType::TTS_Model,		"tts_model" },
		std::pair { PackageType::TTS_Server,	"tts_server" },
	};

	struct PackageInfo
	{
		fig::uuid id;
		PackageType type {};
		TargetPlatform targetPlatform {};
		fig::string name;
		fig::string description;
		fig::string downloadUrl;
		uint64_t dataLength;
		fig::string sha256;
		VersionNumber version;
		fig::path outputDirectory;

		struct FileEntry
		{
			fig::string name;
			fig::string sha256;

			static auto XmlFields() noexcept
			{
				return Fields(
					Element("Name", &FileEntry::name)
						.MustExist(),
					Element("Sha256", &FileEntry::sha256)
				);

				static_assert(IsXmlSerializable<FileEntry>);
			}
		};
		std::vector<FileEntry> archiveFiles; // Archive
		fig::string filename; // Single file

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
					[](auto&& value) { return enum_deserialize(value, TargetPlatformMapping); })
					.MustExist(),
				Element("Name", &PackageInfo::name)
					.MustExist(),
				Element("Description", &PackageInfo::description),
				Element("Url", &PackageInfo::downloadUrl)
					.MustExist(),
				Element("Size", &PackageInfo::dataLength)
					.MustExist(),
				Element("Size", &PackageInfo::dataLength)
					.MustExist(),
				Element("Sha256", &PackageInfo::sha256)
					.MustExist(),
				Element("Version", &PackageInfo::version)
					.MustExist(),
				Element("OutputDirectory", &PackageInfo::outputDirectory)
					.MustExist(),
				Element("File", &PackageInfo::archiveFiles)
					.Collection("Archive"),
				Element("Filename", &PackageInfo::filename)
			);
			static_assert(IsXmlSerializable<PackageInfo>);
		}
	};
}

namespace fig::io
{
	class AsyncDownloader;

	class PackageManager : public fig::data::XmlData<"Packages", 0>
	{
	public:
		PackageManager();

		FileError Init() noexcept;
		void VerifyInstalledPackages();

		fig::optional_cref<fig::data::PackageInfo> GetPackageInfo(const fig::uuid&) const noexcept;

		bool IsPackageInstalled(const fig::uuid& packageId) const;
		bool InstallPackage(const fig::uuid& packageId);

	protected:
		std::vector<fig::data::PackageInfo> _packages;

		enum class PackageInstallState
		{
			NotInstalled,
			PartiallyDownloaded,
			Downloading,
			Downloaded,
			Decompressing,
			Installed,
			Invalid,
			Outdated,
		};
		std::map<fig::uuid, PackageInstallState> _installedPackages;
		std::unique_ptr<AsyncDownloader> _pDownloader;
		std::map<fig::uuid, AsyncDownloadId> _activeInstalls;
	private:
		mutable std::mutex _mutex;

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
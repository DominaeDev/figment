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
			fig::string name;
			fig::path targetPath;
			fig::string sha256;

			static auto XmlFields() noexcept
			{
				return Fields(
					Element("Name", &FileEntry::name)
						.MustExist(),
					Element("Target", &FileEntry::targetPath)
						.MustExist(),
					Element("Sha256", &FileEntry::sha256)
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
				Element("InfoUrl", &PackageInfo::infoUrl)
					.MustExist(),
				Element("Size", &PackageInfo::fileSize)
					.MustExist(),
				Element("Sha256", &PackageInfo::sha256)
					.MustExist(),
				Element("Version", &PackageInfo::version)
					.MustExist(),
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

	class PackageManager : public fig::data::XmlData<"Packages", 0>
	{
	public:
		PackageManager();

		FileError Init() noexcept;
		void VerifyInstalledPackages();

		fig::optional_cref<fig::data::PackageInfo> GetPackage(const fig::uuid&) const noexcept;
		const std::vector<fig::data::PackageInfo>& GetPackages() const noexcept;

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
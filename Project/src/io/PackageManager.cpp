#include <pch.h>
#include "io/PackageManager.h"
#include "io/XmlReader.h"
#include "io/Downloader.h"
#include "io/ZipArchive.h"
#include "io/Xml.h"
#include "util/Hash.h"

using namespace fig::data;

namespace fig::io
{
	constexpr fig::string_view IniStateSection = "Installed";

	PackageManager::PackageManager()
	{
	}

	PackageManager::~PackageManager()
	{
		CancelAll();
		SaveState();
	}

	FileError PackageManager::Init() noexcept
	{
		LoadState();

		// Load package data
		if (auto error = LoadFromXml(fig::path { Constants::Paths::PackagesFileName }); error != FileError::NoError)
		{
			_packages.clear();
			return error;
		}

		return FileError::NoError;
	}

	FileError PackageManager::LoadState()
	{
		auto path = fig::path(Constants::Paths::PackagesFolder) / fig::path("installed.xml");
		XmlReader xml(path);
		if (xml.IsOk())
		{
			if (auto packageNode = xml.GetFirstElement("Package"))
			{
				while (packageNode)
				{
					auto id = (*packageNode)["id"].Get<fig::uuid>();
					auto hash = (*packageNode).GetElement<fig::string>("Sha256", "");

					if (not (id.empty() or hash.empty()))
						_packageHashes[id] = hash;
					packageNode = packageNode->GetNextSibling();
				}
			}
			return FileError::NoError;
		}
		return FileError::NotFound;
	}

	FileError PackageManager::SaveState()
	{
		auto path = fig::path(Constants::Paths::PackagesFolder) / fig::path("installed.xml");

		XmlWriter xml("InstalledPackages");
		for (auto& kvp : _packageHashes)
		{
			auto pPackageNode = xml.AddChild("Package");
			pPackageNode["id"] = (fig::string)kvp.first;
			pPackageNode.SetElementValue("Sha256", kvp.second);
		}

		if (xml.WriteToFile(path))
			return FileError::NoError;
		return FileError::WriteError;
	}

	bool PackageManager::InstallPackage(const fig::uuid& packageId)
	{
		std::scoped_lock _ { _mutex };

		auto itPackage = std::ranges::find(_packages, packageId, [](auto&& p) { return p.id; });
		if (itPackage == std::ranges::cend(_packages))
			return false; // Unknown package
		auto& package = *itPackage;

		PackageState packageState {};
		if (auto itState = _packageStates.find(packageId); itState != _packageStates.cend())
			packageState = itState->second;

		if (packageState == PackageState::Installed)
			return false; // Already installed
	
		if (_finishedInstalls.contains(packageId))
		{
			_activeInstalls.erase(packageId);
			_finishedInstalls.erase(packageId);
		}

		if (_activeInstalls.contains(packageId))
			return false; // Already installing

		auto tempFilename = fig::path { Constants::Paths::TemporaryFolder } / fig::path { (fig::string)package.id };

		if (packageState == PackageState::PartiallyDownloaded)
		{
			std::error_code errorCode;
			uint64_t existingSize = 0;
			if (std::filesystem::exists(tempFilename, errorCode))
			{
				existingSize = std::filesystem::file_size(tempFilename, errorCode);
				if (existingSize == package.fileSize)
					packageState = PackageState::Unverified;
			}
		}

		if (packageState < PackageState::Installed)
		{
			// Start or resume installation
			_activeInstalls.insert(std::make_pair(packageId, Installation {
				.packageInfo = *itPackage,
				.downloader = std::make_unique<Downloader>(),
				.thread = std::make_unique<std::jthread>(std::bind_front(&PackageManager::__InstallPackage, this, packageId)),
			}));
		}

		return true;
	}

	bool PackageManager::UninstallPackage(const fig::uuid& packageId)
	{
		{
			std::scoped_lock _ { _mutex };

			PackageState packageState {};
			if (auto itState = _packageStates.find(packageId); itState != _packageStates.cend())
				packageState = itState->second;

			if (packageState != PackageState::Installed)
				return false; // Already installed

			auto itPackage = std::ranges::find(_packages, packageId, [](auto&& p) { return p.id; });
			if (itPackage == std::ranges::cend(_packages))
				return false; // Unknown package
			auto& package = *itPackage;

			// Delete files
			auto packagesDirectory = fig::path { Constants::Paths::PackagesFolder };
			
			std::vector<fig::path> paths;
			paths.push_back(packagesDirectory / package.targetPath );
			for (auto& entry : package.entries)
				paths.push_back(packagesDirectory / entry.targetPath );

			for (auto& path : paths)
			{
				std::error_code err;
				if (std::filesystem::is_regular_file(path))
					std::filesystem::remove(path, err);
			}

			_packageStates[packageId] = PackageState::NotDownloaded;
			_activeInstalls.erase(packageId);
			_finishedInstalls.erase(packageId);
		}

		__CleanUpTemporaryFiles(packageId);
		return true;
	}

	fig::optional_cref<fig::data::PackageInfo> PackageManager::GetPackage(const fig::uuid& packageId) const noexcept
	{
		if (auto itFind = std::ranges::find(_packages, packageId, [](auto&& p) { return p.id; }); itFind != std::ranges::cend(_packages))
			return *itFind;
		return fig::nullref; // Unknown package
	}

	const std::vector<fig::data::PackageInfo>& PackageManager::GetPackages() const noexcept
	{
		return _packages;
	}

	PackageState PackageManager::GetPackageState(const fig::uuid& packageId) const
	{
		std::scoped_lock _ { _mutex };
		if (auto itFind = _packageStates.find(packageId); itFind != _packageStates.cend())
			return itFind->second;
		return PackageState::Unknown;
	}

	InstallationState PackageManager::GetInstallationState(const fig::uuid& packageId) const
	{
		std::scoped_lock _ { _mutex };

		if (auto itFind = _activeInstalls.find(packageId); itFind != _activeInstalls.cend())
		{
			auto& install = itFind->second;
			if (auto err = std::get_if<Installation::Error>(&install.result))
			{
				switch (*err)
				{
				case Installation::Error::NoError:
					return InstallationState {
						.phase = install.phase,
						.bytesReceived = install.downloader->GetBytesReceived(),
						.bytesTotal = install.downloader->GetBytesTotal(),
					};
				case Installation::Error::DownloadFailed:
					return InstallationState {
						.phase = InstallationPhase::Failed,
						.errorMessage = fig::string { fig::strings::Error::DownloadErrorMessage },
					};
				case Installation::Error::VerificationFailed:
					return InstallationState {
						.phase = InstallationPhase::Failed,
						.errorMessage = fig::string { fig::strings::Error::VerificationErrorMessage },
					};
				default:
				case Installation::Error::Cancelled:
					return InstallationState {
						.phase = InstallationPhase::None,
					};
				}
			}
			else if (auto err = std::get_if<FileError>(&install.result))
			{
				switch (*err)
				{
				case FileError::AccessDenied:
					return InstallationState {
						.phase = InstallationPhase::Failed,
						.errorMessage = fig::string { fig::strings::Error::FileAccessErrorMessage },
					};
				case FileError::DiskFull:
					return InstallationState {
						.phase = InstallationPhase::Failed,
						.errorMessage = fig::string { fig::strings::Error::DiskFullErrorMessage },
					};
				default:
					return InstallationState {
						.phase = InstallationPhase::Failed,
						.errorMessage = std::format(fig::strings::Error::FileErrorMessage, (uint32_t)(*err)),
					};
				}
			}
		}

		return {};
	}

	void PackageManager::CheckInstalledPackages()
	{
		_verificationWorker = std::make_unique<std::jthread>(std::bind_front(&PackageManager::__CheckInstalledPackages, this));
	}

	void PackageManager::__CheckInstalledPackages()
	{
		std::vector<fig::data::PackageInfo> packages;
		std::map<fig::uuid, fig::string> knownHashes;

		{	// Copy package list
			std::scoped_lock lock(_mutex);
			packages.resize(_packages.size());
			std::copy(_packages.begin(), _packages.end(), packages.begin());
			knownHashes = _packageHashes;
		}

		std::map<fig::uuid, PackageState> states;
		for (auto& package : packages)
			states[package.id] = {};
		
		// Check if files exist in their target locations
		for (auto& package : packages)
		{
			bool bOk;
			if (not package.entries.empty())
			{
				bOk = true;
				for (auto& entry : package.entries)
				{
					auto targetPath = fig::path(Constants::Paths::PackagesFolder) / entry.targetPath;
					if (not std::filesystem::exists(targetPath))
					{
						bOk = false;
						break;
					}
				}
			}
			else
			{
				auto targetPath = fig::path(Constants::Paths::PackagesFolder) / package.targetPath;
				bOk = std::filesystem::exists(targetPath);
			}

			if (bOk)
				states[package.id] = PackageState::Unverified;
		}

		// Check for partial downloads
		for (auto& package : packages)
		{
			if (states[package.id] != PackageState::NotDownloaded)
				continue;

			fig::path fullFile = fig::path { Constants::Paths::TemporaryFolder } / fig::path { (fig::string)package.id };
			fig::path partialFile = fig::path { Constants::Paths::TemporaryFolder } / fig::path { std::format("{}.part", (fig::string)package.id) };
			if (std::filesystem::exists(partialFile) or std::filesystem::exists(fullFile))
				states[package.id] = PackageState::PartiallyDownloaded;
		}

		// Check hashes
		for (auto& package : packages)
		{
			if (states[package.id] != PackageState::Unverified)
				continue;

			std::vector<std::pair<fig::uuid, fig::string>> expectedHashes;
			expectedHashes.push_back(std::make_pair(package.id, package.sha256));
			for (auto& entry : package.entries)
			{
				if constexpr (Debugging)
				{
					if (entry.sha256.empty())
						continue; // Allow skipping hash check in debug
				}
				expectedHashes.push_back(std::make_pair(entry.id, entry.sha256));
			}

			bool bKnown = true;
			bool bValid = true;
			for (auto& [id, hash] : expectedHashes)
			{
				if (not knownHashes.contains(id))
				{
					bKnown = false;
					break;
				}
				if (hash != knownHashes[id])
				{
					bValid = false;
					break;
				}
			}
			if (bKnown)
				states[package.id] = bValid ? PackageState::Installed : PackageState::VerificationFailed;
		}

		{	// Store result
			std::scoped_lock lock(_mutex);
			_packageStates.clear();
			_packageStates = states;
		}
	}

	void PackageManager::__InstallPackage(fig::uuid packageId, std::stop_token stopToken)
	{
		// Get state
		fig::observer_ptr<Installation> pInstall;
		{
			std::scoped_lock lock(_mutex);
			pInstall = &_activeInstalls[packageId];
		}
		auto& package = pInstall->packageInfo;

		auto fnFinish = [&](Installation::Result result) {
			std::scoped_lock lock(_mutex);
			pInstall->result = result;
			_finishedInstalls.insert(packageId);

			if (auto err = std::get_if<Installation::Error>(&result))
			{
				if (*err == Installation::Error::NoError)
					_packageStates[packageId] = PackageState::Installed;
				else if (*err == Installation::Error::VerificationFailed)
					_packageStates[packageId] = PackageState::VerificationFailed;
			}
		};

		auto fnSetState = [&](InstallationPhase phase) {
			std::scoped_lock lock(_mutex);
			pInstall->phase = phase;
		};

		auto fnInstall = [&](fig::path source, fig::path target) -> bool {
			std::error_code errorCode {};
			std::filesystem::create_directories(target.parent_path(), errorCode);
			if (errorCode)
				return false;

			std::filesystem::rename(source, target, errorCode);
			return !errorCode;
		};

		bool bShouldDownload = true;
		auto tempDirectory = fig::path { Constants::Paths::TemporaryFolder };
		fig::path tempFilename = tempDirectory / fig::path { (fig::string)package.id };
		fig::path partialFilename = tempDirectory / fig::path { std::format("{}.part", (fig::string)package.id) };

		// Check if download is already complete
		if (std::filesystem::exists(tempFilename) and std::filesystem::file_size(tempFilename) == package.fileSize)
			bShouldDownload = false;

		// Download
		if (bShouldDownload)
		{
			fnSetState(InstallationPhase::Downloading);

			auto downloadError = pInstall->downloader->Download(package.downloadUrl, tempFilename, stopToken);
			switch (downloadError)
			{
			case DownloadError::WriteAccessError:
				fnFinish(FileError::AccessDenied);
				return;
			case DownloadError::DiskFullError:
				fnFinish(FileError::DiskFull);
				return;
			case DownloadError::FileError:
				fnFinish(FileError::WriteError);
				return;
			case DownloadError::NoError:
			case DownloadError::Cancelled:
				break;
			}

			if (downloadError == DownloadError::FileError)
			{
				fnFinish(FileError::WriteError);
				return;
			}
			else if (downloadError != DownloadError::NoError and downloadError != DownloadError::Cancelled)
			{
				fnFinish(Installation::Error::DownloadFailed);
				return;
			}
		}

		if (stopToken.stop_requested())
		{
			fnFinish(Installation::Error::Cancelled);
			return;
		}

		if (not std::filesystem::exists(tempFilename))
		{
			fnFinish(FileError::NotFound);
			return;
		}

		fnSetState(InstallationPhase::Verifying);

		// Verify hash
		fig::hash hash;
		if (not package.sha256.empty())
			hash = GetHash(tempFilename, stopToken);

		if (stopToken.stop_requested())
		{
			fnFinish(Installation::Error::Cancelled);
			return;
		}

		bool bVerified = (fig::string)hash == package.sha256;
		if constexpr (Debugging)
		{
			bVerified |= package.sha256.empty();
		}

		if (not bVerified)
		{
			__CleanUpTemporaryFiles(packageId);
			fnFinish(Installation::Error::VerificationFailed);
			return;
		}

		if (not hash.empty())
		{
			// Store hash
			std::scoped_lock lock(_mutex);
			_packageHashes[packageId] = (fig::string)hash;
		}

		if constexpr (Disabled) // Somewhat redundant after a successful SHA256, and getting a precise byte count for every download is a pain
		{
			// Verify file size
			if (std::filesystem::file_size(tempFilename) != package.fileSize)
			{
				fnFinish(Installation::Error::VerificationFailed);
				return;
			}
		}

		// Decompress archive
		if (not package.entries.empty())
		{
			fnSetState(InstallationPhase::Decompressing);
			bool bValidContents = true;

			ZipArchive zip;
			if (zip.Open(tempFilename) == ZipError::NoError)
			{
				for (auto& packageEntry : package.entries)
				{
					if (stopToken.stop_requested())
					{
						fnFinish(Installation::Error::Cancelled);
						return;
					}

					if constexpr (Disabled) // Somewhat redundant after a successful SHA256, and getting a precise byte count for every download is a pain
					{
						// Verify file size
						if (packageEntry.fileSize != 0ULL)
						{
							if (auto try_entry = zip.GetEntry(packageEntry.name))
							{
								if ((*try_entry).uncompressedSize != packageEntry.fileSize)
								{
									bValidContents = false;
									break;
								}
							}
							else
							{
								bValidContents = false;
								break;
							}
						}
					}

					auto entryTempFilename = tempDirectory / fig::path { (fig::string)packageEntry.id };
					if (auto error = zip.Extract(packageEntry.name, entryTempFilename); error != ZipError::NoError)
					{
						bValidContents = false;
						break;
					}
				}
			}
			else
			{
				bValidContents = false;
			}

			if (not bValidContents)
			{
				__CleanUpTemporaryFiles(packageId);
				fnFinish(Installation::Error::VerificationFailed);
				return;
			}
		}

		// Copy files to their target location
		fnSetState(InstallationPhase::Installing);

		if (package.entries.empty())
		{
			auto targetPath = fig::path(Constants::Paths::PackagesFolder) / package.targetPath;
			if (not fnInstall(tempFilename, targetPath))
			{
				fnFinish(FileError::WriteError);
				return;
			}
		}
		else
		{
			for (auto& entry : package.entries)
			{
				auto targetPath = fig::path(Constants::Paths::PackagesFolder) / entry.targetPath;
				auto entryTempFilename = tempDirectory / fig::path { (fig::string)entry.id };
				if (not fnInstall(entryTempFilename, targetPath))
				{
					fnFinish(FileError::WriteError);
					return;
				}
			}
		}

		fnSetState(InstallationPhase::Completed);
		__CleanUpTemporaryFiles(packageId);
		fnFinish(Installation::Error::NoError);
	}

	void PackageManager::__CleanUpTemporaryFiles(fig::uuid packageId)
	{
		std::scoped_lock lock(_mutex);

		auto itPackage = std::ranges::find(_packages, packageId, [](auto&& p) { return p.id; });
		if (itPackage == std::ranges::cend(_packages))
			return; // Unknown package
		auto& package = *itPackage;

		auto tempDirectory = fig::path { Constants::Paths::TemporaryFolder };
		std::vector<fig::path> paths;
		paths.push_back(tempDirectory / fig::path { (fig::string)package.id });
		paths.push_back(tempDirectory / fig::path { std::format("{}.part", (fig::string)package.id) });
		for (auto& entry : package.entries)
			paths.push_back(tempDirectory / fig::path { (fig::string)entry.id });

		// Clean up
		for (auto& path : paths)
		{
			std::error_code err;
			std::filesystem::remove(path, err);
		}
	}

	void PackageManager::CancelAll()
	{
		for (auto& kvp : _activeInstalls)
			kvp.second.thread->request_stop();
		_activeInstalls.clear();
	}

	bool PackageManager::CancelInstall(fig::uuid packageId)
	{
		if (auto itFind = _activeInstalls.find(packageId); itFind != _activeInstalls.cend())
		{
			itFind->second.thread->request_stop();
			_activeInstalls.erase(itFind);
			return true;
		}
		return false;
	}
}
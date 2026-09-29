#include <pch.h>
#include "io/PackageManager.h"
#include "io/XmlReader.h"
#include "io/AsyncDownloader.h"
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
		if (auto error = LoadFromXml(fig::path { Constants::Paths::PackagesFolder } / fig::path { Constants::Paths::PackagesFileName }); error != FileError::NoError)
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

		// Is downloading?
		if (auto itFind = _activeInstalls.find(packageId); itFind != _activeInstalls.cend())
		{
			auto& install = itFind->second;
			switch (install.error)
			{
			case Installation::Error::NoError:
				return InstallationState {
					.phase = install.phase,
					.bytesReceived = install.downloader->GetBytesReceived(),
					.bytesTotal = install.downloader->GetBytesTotal(),
				};
			case Installation::Error::Cancelled:
				return InstallationState {
					.phase = InstallationPhase::None,
				};
			default:
				return InstallationState {
					.phase = InstallationPhase::Failed,
				};
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
		auto& downloader = *pInstall->downloader.get();

		auto fnFinish = [&](Installation::Error error) {
			std::scoped_lock lock(_mutex);
			pInstall->error = error;
			_finishedInstalls.insert(packageId);

			if (error == Installation::Error::NoError)
				_packageStates[packageId] = PackageState::Installed;
			else if (error == Installation::Error::VerificationFailed)
				_packageStates[packageId] = PackageState::VerificationFailed;
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
		fig::path tempFilename = fig::path { Constants::Paths::TemporaryFolder } / fig::path { (fig::string)package.id };
		fig::path partialFilename = fig::path { Constants::Paths::TemporaryFolder } / fig::path { std::format("{}.part", (fig::string)package.id) };

		// Check if download is already complete
		if (std::filesystem::exists(tempFilename) and std::filesystem::file_size(tempFilename) == package.fileSize)
			bShouldDownload = false;

		// Download
		if (bShouldDownload)
		{
			fnSetState(InstallationPhase::Downloading);

			pInstall->downloadError = downloader.Download(package.downloadUrl, tempFilename, stopToken);
			if (pInstall->downloadError != DownloadError::NoError)
			{
				fnFinish(Installation::Error::DownloadError);
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
			pInstall->fileError = FileError::NotFound;
			fnFinish(Installation::Error::FileError);
			return;
		}

		// Decompress archive
		if (not package.entries.empty())
		{
			fnSetState(InstallationPhase::Decompressing);

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

					// Verify file size
					if (packageEntry.fileSize != 0ULL)
					{
						if (auto try_entry = zip.GetEntry(packageEntry.name))
						{
							if ((*try_entry).uncompressedSize != packageEntry.fileSize)
							{
								//! @todo: Delete file
								fnFinish(Installation::Error::VerificationFailed);
								return;
							}
						}
						else
						{
							fnFinish(Installation::Error::VerificationFailed);
							return;
						}
					}

					auto entryTempFilename = fig::path { Constants::Paths::TemporaryFolder } / fig::path { (fig::string)packageEntry.id };
					if (auto error = zip.Extract(packageEntry.name, entryTempFilename); error != ZipError::NoError)
					{
						//! @todo: Delete file
						fnFinish(Installation::Error::VerificationFailed);
						return;
					}
				}
			}
			else
			{
				//! @todo: Delete file
				fnFinish(Installation::Error::VerificationFailed);
				return;
			}
		}

		fnSetState(InstallationPhase::Verifying);
		bool bVerified = false;

		// Verify file size
		if (std::filesystem::file_size(tempFilename) != package.fileSize)
		{
			fnFinish(Installation::Error::VerificationFailed);
			return;
		}

		// Verify hash(es)
		std::vector<std::pair<fig::path, fig::string>> expectedHashes;
		expectedHashes.push_back(std::make_pair(tempFilename, package.sha256));
		for (auto& entry : package.entries)
		{
			auto entryTempFilename = fig::path { Constants::Paths::TemporaryFolder } / fig::path { (fig::string)entry.id };
			expectedHashes.push_back(std::make_pair(entryTempFilename, entry.sha256));
		}

		for (auto& [filename, expected] : expectedHashes)
		{
			fig::hash hash;
			if (not expected.empty())
				hash = GetHash(filename, stopToken);

			if (stopToken.stop_requested())
			{
				fnFinish(Installation::Error::Cancelled);
				return;
			}

			bVerified = (fig::string)hash == expected;
			if constexpr (Debugging)
			{
				// Allow skipping hash check in debug
				bVerified |= expected.empty();
			}

			if (bVerified and not hash.empty())
			{
				// Store hash
				std::scoped_lock lock(_mutex);
				_packageHashes[packageId] = (fig::string)hash;
			}

			if (not bVerified)
			{
				//! @todo: Delete file
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
				fnFinish(Installation::Error::FileError);
				return;
			}
		}
		else
		{
			for (auto& entry : package.entries)
			{
				auto targetPath = fig::path(Constants::Paths::PackagesFolder) / entry.targetPath;
				auto entryTempFilename = fig::path { Constants::Paths::TemporaryFolder } / fig::path { (fig::string)entry.id };
				if (not fnInstall(entryTempFilename, targetPath))
				{
					fnFinish(Installation::Error::FileError);
					return;
				}
			}
		}

		fnSetState(InstallationPhase::Completed);
		fnFinish(Installation::Error::NoError);
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
#include <pch.h>
#include "io/PackageManager.h"
#include "io/XmlReader.h"
#include "io/AsyncDownloader.h"
#include "io/IniFile.h"
#include "util/Hash.h"

using namespace fig::data;

namespace fig::io
{
	constexpr fig::string_view IntermediaryPath = "temp/";
	constexpr fig::string_view IniStateSection = "Installed";

	PackageManager::PackageManager()
	{
		_pDownloader = std::make_unique<AsyncDownloader>();
	}

	PackageManager::~PackageManager()
	{
		SaveState();
	}

	FileError PackageManager::Init() noexcept
	{
		// Load package data
		if (auto error = LoadFromXml(fig::path { "packages/packages.xml" }); error != FileError::NoError)
			return error;

		// Load states
		IniFile ini;
		if (auto error = ini.Load("packages/installed.ini"); error == IniError::NoError)
		{
			for (auto& package : _packages)
			{
				auto key = (fig::string)package.id;
				if (ini.HasKey(IniStateSection, key))
					_packageHashes[package.id] = ini.Get<fig::string>(IniStateSection, key).value_or("");
			}
		}

		return FileError::NoError;
	}

	FileError PackageManager::SaveState()
	{
		IniFile ini;
		for (auto& kvp : _packageHashes)
			ini.Set(IniStateSection, (fig::string)kvp.first, kvp.second);

		if (auto error = ini.Save("packages/installed.ini"); error == IniError::NoError)
			return FileError::NoError;
		else
		{
			switch (error)
			{
			case IniError::FileAccessDenied:
				return FileError::AccessDenied;
			default:
				return FileError::WriteError;
			}
		}
	}

	bool PackageManager::IsPackageInstalled(const fig::uuid& packageId) const
	{
		std::scoped_lock _ { _mutex };
		return _packageStates.contains(packageId);
	}

	bool PackageManager::InstallPackage(const fig::uuid& packageId)
	{
		auto itFind = std::ranges::find(_packages, packageId, [](auto&& p) { return p.id; });
		if (itFind == std::ranges::cend(_packages))
			return false; // Unknown package

		if (IsPackageInstalled(packageId))
			return false; // Already installed
	
		if (_activeInstalls.contains(packageId))
			return false; // Already installing

		auto& package = *itFind;
		
		AsyncDownloadId downloadId = _pDownloader->Start(package.downloadUrl, fig::path { std::format("{}{}", IntermediaryPath, (fig::string)package.id) }, [](AsyncDownloadId, DownloadError) {
			int k = 0;
		});

		_activeInstalls[packageId] = downloadId;
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

	void PackageManager::VerifyInstalledPackages()
	{
		_verificationWorker = std::make_unique<std::jthread>(std::bind_front(&PackageManager::__Verify, this));
	}

	void PackageManager::__Verify()
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
					if (not std::filesystem::exists(entry.targetPath))
					{
						bOk = false;
						break;
					}
				}
			}
			else
				bOk = std::filesystem::exists(package.targetPath);

			if (bOk)
				states[package.id] = PackageState::Unverified;
		}

		// Check for partial downloads
		for (auto& package : packages)
		{
			fig::path tempPath { std::format("{}{}.part", IntermediaryPath, (fig::string)package.id) };
			if (std::filesystem::exists(tempPath))
				states[package.id] = PackageState::PartiallyDownloaded;
		}

		// Check hashes
		for (auto& package : packages)
		{
			if (states[package.id] != PackageState::Unverified)
				continue;

			if (not package.entries.empty())
			{
				bool bKnown = true;
				bool bValid = true;
				for (auto& entry : package.entries)
				{
					if (not knownHashes.contains(entry.id))
					{
						bKnown = false;
						break;
					}
					if (package.sha256 != knownHashes[entry.id])
					{
						bValid = false;
						break;
					}
				}
				if (bKnown)
					states[package.id] = bValid ? PackageState::Installed : PackageState::Invalid;
			}
			else
			{
				bool bKnown = knownHashes.contains(package.id);
				bool bValid = package.sha256 == knownHashes[package.id];
				if (bKnown)
					states[package.id] = bValid ? PackageState::Installed : PackageState::Invalid;
			}
		}

		{	// Store result
			std::scoped_lock lock(_mutex);
			_packageStates.clear();
			_packageStates = states;
		}
	}

}
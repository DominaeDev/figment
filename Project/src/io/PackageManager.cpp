#include <pch.h>
#include "io/PackageManager.h"
#include "io/XmlReader.h"
#include "io/AsyncDownloader.h"

using namespace fig::data;

namespace fig::io
{
	PackageManager::PackageManager()
	{
		_pDownloader = std::make_unique<AsyncDownloader>();
	}

	FileError PackageManager::Init() noexcept
	{
		return LoadFromXml(fig::path { "resources/packages/packages.xml" });
	}

	bool PackageManager::IsPackageInstalled(const fig::uuid& packageId) const
	{
		std::scoped_lock _ { _mutex };
		return _installedPackages.contains(packageId);
	}

	void PackageManager::VerifyInstalledPackages()
	{
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
		
		AsyncDownloadId downloadId = _pDownloader->Start(package.downloadUrl, fig::path { std::format("temp/{}", (fig::string)package.id) }, [](AsyncDownloadId, DownloadError) {
			int k = 0;
		});

		_activeInstalls[packageId] = downloadId;

		return true;
	}

	fig::optional_cref<fig::data::PackageInfo> PackageManager::GetPackageInfo(const fig::uuid& packageId) const noexcept
	{
		if (auto itFind = std::ranges::find(_packages, packageId, [](auto&& p) { return p.id; }); itFind != std::ranges::cend(_packages))
			return *itFind;
		return fig::nullref; // Unknown package
	}
}
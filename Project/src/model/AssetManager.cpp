#include <pch.h>
#include "model/AssetManager.h"
#include "model/UserManager.h"
#include "model/AppState.h"
#include "util/Common.h"
#include "util/BinaryWriter.h"

using namespace fig::common_util;

namespace fig::fs
{
	Asset& AssetManager::CreateAsset(AssetType type, fig::bytes&& data) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {
			.id = id,
			.asset_type = type,
			.data = std::move(data),
			.needSave = true,
		};
		return newAsset;
	}

	Asset& AssetManager::CreateAsset(AssetType type, fig::byte_span data) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {
			.id = id,
			.asset_type = type,
			.needSave = true,
		};

		newAsset.data.resize(data.size());
		std::memcpy(newAsset.data.data(), data.data(), data.size());
		return newAsset;
	}

	std::optional<std::reference_wrapper<Asset>> AssetManager::FindAsset(const fig::uuid& id) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend())
			return std::make_optional<std::reference_wrapper<Asset>>(static_cast<Asset&>(itFind->second));
		return std::nullopt;
	}

	void AssetManager::SaveAll()
	{
		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (not asset.needSave)
				continue;

			if (WriteAsset(asset))
				asset.needSave = false;
		}
	}

	bool AssetManager::WriteAsset(const Asset& asset) const
	{
		auto& userMngr = ApplicationState::GetUserManager();
		if (not userMngr.IsSignedIn())
			return false;
		
		auto const& authKey = userMngr.GetActiveAuthKey();
		auto const& profile = userMngr.GetActiveProfile();

		if (not profile.has_value())
			return false; // Error
		
		auto file = asset.ToFile();
		BinaryWriter writer(profile.value().get().name, authKey);
		auto error = writer.WriteFile(file);
		return error == FileError::NoError;
	}
}
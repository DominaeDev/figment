#include <pch.h>
#include "model/AssetManager.h"
#include "model/UserManager.h"
#include "model/AppState.h"
#include "util/Common.h"
#include "util/BinaryWriter.h"
#include "util/Xml.h"
#include <filesystem>
#include <format>

using namespace fig::common_util;

namespace fig::fs
{
	AssetManager::AssetManager(const UserManager& userMngr)
	{
		UserProfile& profile = userMngr.GetActiveProfile().value();
		_profileAuthKey = userMngr.GetActiveAuthKey();
		_profileID = profile.id;
		_profilePath = profile.GetPath();

		if (not LoadIndex())
		{
			Log(std::format("No asset index found for profile {}.", profile.name));
		}
	}

	bool AssetManager::LoadIndex()
	{
		auto const path = _profilePath / std::format("{}.{}", Constants::Paths::ProfileIndexFileName, Constants::Paths::ProfileIndexFileExt);

		fig::XmlReader xml(path.u8string(), "Assets");
		if (not xml.IsOk())
			return false; // Invalid document type

		_assets.clear();

		auto optAsset = xml.GetFirstElement("Asset");
		while (optAsset)
		{
			auto& assetNode = optAsset.value();

			Asset asset {};
			asset.id = assetNode.GetElementUUID("ID").value_or({});
			asset.parent_id = assetNode.GetElementUUID("ParentID").value_or({});
			auto [type, subtype] = AssetTypeFromString(assetNode.GetElementText("Type").value_or(""));
			asset.asset_type = type;
			asset.asset_subtype = subtype;
			asset.data_format = DataFormatFromString(assetNode.GetElementText("Format").value_or(""));
			asset.status = FileStatus::NotLoaded;
			if (!asset.id.empty())
				_assets[asset.id] = asset;
			optAsset = assetNode.GetNextSibling();
		}

		return true;
	}

	bool AssetManager::SaveIndex() const
	{
		auto const path = _profilePath / std::format("{}.{}", Constants::Paths::ProfileIndexFileName, Constants::Paths::ProfileIndexFileExt);

		XmlWriter xml("Assets");
		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			auto assetNode = xml.AddChild("Asset");
			assetNode.SetElement("ID", asset.id);
			assetNode.SetElement("ParentID", asset.parent_id);
			assetNode.SetElement("Type", AssetTypeToString(asset.asset_type, asset.asset_subtype));
			assetNode.SetElement("Format", DataFormatToString(asset.data_format));
		}

		return xml.Save(path.u8string());
	}

	Asset& AssetManager::CreateAsset(AssetType type, fig::bytes&& data) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = _profileID;
		newAsset.asset_type = type;
		newAsset.data = std::move(data); // Move data
		newAsset.status = FileStatus::Modified;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());
		return newAsset;
	}

	Asset& AssetManager::CreateEmptyAsset(AssetType type) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = _profileID;
		newAsset.asset_type = type;
		newAsset.status = FileStatus::PartiallyLoaded;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());
		return newAsset;
	}

	Asset& AssetManager::CreateAsset(AssetType type, fig::byte_span data) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = _profileID;
		newAsset.asset_type = type;
		newAsset.status = FileStatus::Modified;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());

		// Copy data
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
			if (asset.status != FileStatus::Modified)
				continue;

			if (WriteAsset(asset))
				asset.status = FileStatus::FullyLoaded;
		}
		SaveIndex();
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
		BinaryWriter writer(authKey);
		auto error = writer.WriteFile(profile.value().get().GetPath(), file);
		return error == FileError::NoError;
	}
}
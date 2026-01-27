#include <pch.h>
#include "model/AssetManager.h"
#include "model/UserManager.h"
#include "model/AppState.h"
#include "util/Common.h"
#include "util/BinaryWriter.h"
#include "util/BinaryReader.h"
#include "util/Xml.h"
#include <filesystem>
#include <format>
#include <ranges>

using namespace fig::common_util;

namespace fig::fs
{
	AssetManager::AssetManager(const UserManager& userMngr)
	{
		auto const& profile = userMngr.GetActiveProfile();
		_profileAuthKey = userMngr.GetActiveAuthKey();
		_profileID = profile.id;
		_profilePath = profile.GetPath();

		if (not LoadIndex())
			Log(std::format("No asset index found for profile '{}'.", profile.name));

		LoadAssetMetaData();
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

	Asset& AssetManager::CreateEmptyAsset(AssetType type, const fig::uuid& parent) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = not parent.empty() ? parent : _profileID;
		newAsset.asset_type = type;
		newAsset.status = FileStatus::PartiallyLoaded;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());
		return newAsset;
	}

	Asset& AssetManager::CreateAsset(AssetType type, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = not parent.empty() ? parent : _profileID;
		newAsset.asset_type = type;
		newAsset.data = std::move(data); // Move data
		newAsset.status = FileStatus::Modified;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());
		return newAsset;
	}

	Asset& AssetManager::CreateAsset(AssetType type, fig::byte_span data, const fig::uuid& parent) noexcept
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

	std::optional<AssetRef> AssetManager::FindAsset(const fig::uuid& id) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend())
			return std::make_optional<AssetRef>(static_cast<Asset&>(itFind->second));
		return std::nullopt;
	}

	void AssetManager::SaveModified()
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
		auto file = asset.ToFile();
		BinaryWriter writer(_profileAuthKey);
		auto error = writer.WriteFile(_profilePath, file);
		return error == FileError::NoError;
	}

	bool AssetManager::LoadAssetMetaData()
	{
		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (asset.status > FileStatus::NotLoaded)
				continue;

			BinaryReader reader(_profilePath, _profileAuthKey);
			if (auto file = reader.ReadFile(asset.GetFileName(), false))
			{
				asset.FromFile(std::move(file.value()));
				asset.status = FileStatus::PartiallyLoaded;
			}
			else
			{
				asset.status = FileStatus::Invalid;
			}
		}
		return true;
	}

	std::expected<AssetRef, FileError> AssetManager::LoadAsset(const fig::uuid& id) noexcept
	{
		auto findAsset = FindAsset(id);
		if (not findAsset.has_value())
			return std::unexpected(FileError::FileNotFound);
		
		Asset& asset = findAsset.value();
		if (asset.status == FileStatus::FullyLoaded)
			return asset;

		if (asset.status == FileStatus::Invalid)
			return std::unexpected(FileError::ReadError);

		BinaryReader reader(_profilePath, _profileAuthKey);
		if (auto file = reader.ReadFile(asset.GetFileName()))
		{
			asset.FromFile(std::move(file.value()));
			asset.status = FileStatus::FullyLoaded;
			return asset;
		}
		else
		{
			asset.status = FileStatus::Invalid;
			return std::unexpected(FileError::ReadError);
		}
	}
}
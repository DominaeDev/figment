#include <pch.h>
#include "model/ContentManager.h"
#include "model/AssetManager.h"

namespace fig::io
{
	UserContentManager::UserContentManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey)
	{
		_pAssetMngr = std::make_unique<AssetManager>(profile, authKey);

		LoadAll();
	}

	UserContentManager::~UserContentManager()
	{
		_pAssetMngr->SaveModified();
	}

	void UserContentManager::LoadAll()
	{
		DEBUG_MEASURE_BEGIN("UserContentManager::LoadAll");

		auto& assetMngr = *_pAssetMngr;

		// Load characters
		for (auto& asset : _pAssetMngr->GetCharacterAssets())
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				CharacterData character;
				if (character.LoadFromXml(asset.AsStringView()) == FileError::NoError)
				{
					character.createdAt = asset.GetCreatedAt();
					character.updatedAt = asset.GetUpdatedAt();
					_characters[asset.id] = std::move(character);
				}
			}
			else
				continue; // Skip
		}

		// Load scenarios
		for (auto& asset : _pAssetMngr->GetScenarioAssets())
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				ScenarioData scenario;
				if (scenario.LoadFromXml(asset.AsString()) == FileError::NoError)
					_scenarios[asset.id] = std::move(scenario);
			}
			else
				continue; // Skip
		}

		DEBUG_MEASURE_END();
	}

	std::optional<fig::io::CharacterData> UserContentManager::GetCharacter(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _characters.find(id); itFind != _characters.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}

	std::optional<fig::io::ScenarioData> UserContentManager::GetScenario(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _scenarios.find(id); itFind != _scenarios.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}

	std::optional<std::reference_wrapper<fig::io::CardMetaData>> UserContentManager::GetMetaData(const fig::uuid& id, bool bIgnoreCache) noexcept
	{
		if (not bIgnoreCache)
		{
			if (auto itFind = _metaData.find(id); itFind != _metaData.cend())
				return std::make_optional(std::ref(itFind->second));
		}

		if (auto tryAsset = _pAssetMngr->FindAsset(id))
		{
			auto& asset = tryAsset.value().get();
			CardMetaData metaData = CardMetaData::FromJson(asset.settings).value_or({});
			metaData.createdAt = asset.GetCreatedAt();
			metaData.updatedAt = asset.GetUpdatedAt();
			metaData.lastUsedAt = asset.GetLastUsedAt();

			if (auto itFind = _characters.find(id); itFind != _characters.cend())
			{
				auto& character = itFind->second;
				metaData.name = character.shortName;
				metaData.chatCount = static_cast<uint32_t>(std::ranges::count_if(_pAssetMngr->GetAssets(), [&id](auto&& a) { return a.asset_type == AssetType::ChatInstance and a.parent_id == id; }));
			}
			_metaData[id] = metaData;
			return std::make_optional(std::ref(_metaData.at(id)));
		}

		return std::nullopt;
	}

	template <CardMetaData::Flag E>
	bool UserContentManager::MarkFlag(const fig::uuid& assetId, bool value)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			auto& asset = tryAsset.value().get();
			if (auto tryMeta = GetMetaData(assetId))
			{
				auto& meta = tryMeta.value().get();
				value ? meta.flags.Set(E) : meta.flags.Unset(E);

				asset.SetSettings(CardMetaData::ToJson(meta));
				return true;
			}
		}
		return false;
	}

	bool UserContentManager::MarkNew(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<CardMetaData::Flag::New>(assetId, value);
	}

	bool UserContentManager::MarkImported(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<CardMetaData::Flag::Imported>(assetId, value);
	}

	bool UserContentManager::MarkFavorite(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<CardMetaData::Flag::Favorite>(assetId, value);
	}

	bool UserContentManager::MarkHidden(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<CardMetaData::Flag::Hidden>(assetId, value);
	}

	size_t UserContentManager::ImportCharactersInDirectory(const fig::path& directory)
	{
		auto imported = _pAssetMngr->ImportCharactersInDirectory(directory, AssetManager::CharacterDataFormat::TavernV2);
		for (auto& import : imported)
		{
			auto& asset = import.get();
			MarkNew(asset.id);
			MarkImported(asset.id);
		}
		return imported.size();
	}

	std::expected<AssetRef, FileError> UserContentManager::ImportCharacter(const fig::path& filename)
	{
		if (auto imported = _pAssetMngr->ImportCharacter(filename, AssetManager::CharacterDataFormat::TavernV2))
		{
			auto& asset = imported.value().get();
			MarkNew(asset.id);
			MarkImported(asset.id);
			return std::ref(asset);
		}
		else
			return std::unexpected(imported.error());
	}

	std::expected<AssetRef, FileError> UserContentManager::ImportScenario(const fig::path& filename)
	{
		if (auto imported = _pAssetMngr->ImportScenario(filename))
		{
			auto& asset = imported.value().get();
			MarkNew(asset.id);
			MarkImported(asset.id);
			return std::ref(asset);
		}
		else
			return std::unexpected(imported.error());
	}

	AssetManager& UserContentManager::GetAssetManager()
	{
		return *_pAssetMngr;
	}

	void UserContentManager::SaveModified()
	{
		_pAssetMngr->SaveModified();
	}
}
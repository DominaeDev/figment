#include <pch.h>
#include "model/ContentDatabase.h"
#include "model/AssetManager.h"

using namespace fig::data;

namespace fig::fs
{
	ContentDatabase::ContentDatabase(AssetManager& assetMngr)
	{
		_pAssetMngr = &assetMngr;

		LoadAllCharacters();
	}

	void ContentDatabase::LoadAllCharacters()
	{
		auto& assetMngr = *_pAssetMngr;
		auto assets = _pAssetMngr->GetAssets()
			| std::views::filter([](auto& a) { return a.asset_type == AssetType::Character; });
		
		for (auto& asset : assets)
		{
			assetMngr.LoadAsset(asset);

			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				CharacterData character;
				if (character.LoadFromXml(asset.AsString()))
					_characters[asset.id] = std::move(character);
			}
			else
				continue; // Skip

		}
	}

}
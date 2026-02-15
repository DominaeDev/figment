#include <pch.h>
#include "model/ContentDatabase.h"
#include "model/AssetManager.h"

using namespace fig::data;

namespace fig::fs
{
	ContentDatabase::ContentDatabase(AssetManager& assetMngr)
	{
		_pAssetMngr = &assetMngr;

		LoadAll();
	}

	void ContentDatabase::LoadAll()
	{
		auto& assetMngr = *_pAssetMngr;

		// Load characters
		for (auto& asset : _pAssetMngr->GetAllCharacters())
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

		// Load scenarios
		for (auto& asset : _pAssetMngr->GetAllScenarios())
		{
			assetMngr.LoadAsset(asset);

			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				ScenarioData scenario;
				if (scenario.LoadFromXml(asset.AsString()))
					_scenarios[asset.id] = std::move(scenario);
			}
			else
				continue; // Skip
		}
	}

	std::optional<fig::data::CharacterData> ContentDatabase::GetCharacter(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _characters.find(id); itFind != _characters.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}

	std::optional<fig::data::ScenarioData> ContentDatabase::GetScenario(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _scenarios.find(id); itFind != _scenarios.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}
}
#include <pch.h>
#include "model/ContentDatabase.h"
#include "model/AssetManager.h"

namespace fig::io
{
	ContentDatabase::ContentDatabase(AssetManager& assetMngr)
	{
		_pAssetMngr = &assetMngr;

		LoadAll();
	}

	void ContentDatabase::LoadAll()
	{
		DEBUG_MEASURE_BEGIN("ContentDatabase::LoadAll");

		auto& assetMngr = *_pAssetMngr;

		// Load characters
		for (auto& asset : _pAssetMngr->GetAllCharacters())
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				CharacterData character;
				if (character.LoadFromXml(asset.AsStringView()))
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
		for (auto& asset : _pAssetMngr->GetAllScenarios())
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				ScenarioData scenario;
				if (scenario.LoadFromXml(asset.AsString()))
					_scenarios[asset.id] = std::move(scenario);
			}
			else
				continue; // Skip
		}

		DEBUG_MEASURE_END();

	}

	std::optional<fig::io::CharacterData> ContentDatabase::GetCharacter(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _characters.find(id); itFind != _characters.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}

	std::optional<fig::io::ScenarioData> ContentDatabase::GetScenario(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _scenarios.find(id); itFind != _scenarios.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}
}
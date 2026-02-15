#ifndef CHARACTER_DATABASE_H__
#define CHARACTER_DATABASE_H__
#pragma once

#include "model/CharacterData.h"
#include "model/ScenarioData.h"

namespace fig::fs
{
	class AssetManager;

	class ContentDatabase
	{
	public:
		ContentDatabase(AssetManager& assetMngr);

		std::optional<fig::data::CharacterData> GetCharacter(const fig::uuid& id) const noexcept;
		auto GetCharacters() const noexcept { return _characters | std::views::values; }

		std::optional<fig::data::ScenarioData> GetScenario(const fig::uuid& id) const noexcept;
		auto GetScenarios() const noexcept { return _scenarios | std::views::values; }

	protected:
		void LoadAll();

	private:
		AssetManager* _pAssetMngr {};

		std::map<fig::uuid, fig::data::CharacterData> _characters;
		std::map<fig::uuid, fig::data::ScenarioData> _scenarios;
	};
}
#endif
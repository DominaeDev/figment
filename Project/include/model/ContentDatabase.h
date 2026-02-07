#ifndef CHARACTER_DATABASE_H__
#define CHARACTER_DATABASE_H__
#pragma once

#include "model/Character.h"

namespace fig::fs
{
	class AssetManager;

	class ContentDatabase
	{
	public:
		ContentDatabase(AssetManager& assetMngr);

		auto GetCharacters() const noexcept { return _characters | std::views::values; }

	protected:
		void LoadAllCharacters();

	private:
		AssetManager* _pAssetMngr {};

		std::map<fig::uuid, fig::data::CharacterData> _characters;
	};
}
#endif
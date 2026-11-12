#pragma once

#include "io/AssetFolder.h"

namespace fig::data
{
	class Character;
	class Scenario;
	class World;
}

namespace fig::gui
{
	template<typename T>
	struct CardTypeOf;

	template<>
	struct CardTypeOf<fig::data::Character>
	{
		using Type = class CharacterCard;
		using ListType = class CharacterCardList;
		static constexpr fig::io::AssetFolderType FolderType = fig::io::AssetFolderType::Character;
	};

	template<>
	struct CardTypeOf<fig::data::Scenario>
	{
		using Type = class ScenarioCard;
		using ListType = class StoryCardList;
		static constexpr fig::io::AssetFolderType FolderType = fig::io::AssetFolderType::Story;
	};

	template<>
	struct CardTypeOf<fig::data::World>
	{
		using Type = class WorldCard;
		using ListType = class WorldCardList;
		static constexpr fig::io::AssetFolderType FolderType = fig::io::AssetFolderType::World;
	};
}
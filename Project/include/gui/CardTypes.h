#pragma once

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
	};

	template<>
	struct CardTypeOf<fig::data::Scenario>
	{
		using Type = class ScenarioCard;
		using ListType = class StoryCardList;
	};

	template<>
	struct CardTypeOf<fig::data::World>
	{
		using Type = class WorldCard;
		using ListType = class WorldCardList;
	};
}
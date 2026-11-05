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
	};

	template<>
	struct CardTypeOf<fig::data::Scenario>
	{
		using Type = class ScenarioCard;
	};

	template<>
	struct CardTypeOf<fig::data::World>
	{
		using Type = class WorldCard;
	};
}
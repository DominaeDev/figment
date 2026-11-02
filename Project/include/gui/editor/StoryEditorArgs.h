#pragma once

#include "Figment.h"

namespace fig::data
{
	class Scenario;
	class Character;
}

namespace fig::gui
{
	struct StoryEditorArgs
	{
		fig::uuid assetId;
		fig::observer_ptr<fig::data::Scenario> pScenario;
		fig::observer_ptr<const fig::data::Character> pCharacter;
	};
}
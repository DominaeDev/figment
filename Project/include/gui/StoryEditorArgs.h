#pragma once

#include "Figment.h"
#include "data/Scenario.h"

namespace fig::gui
{
	struct StoryEditorArgs
	{
		fig::uuid assetId;
		fig::observer_ptr<fig::data::Scenario> pScenario;
		bool bCharacterStory {};
	};
}
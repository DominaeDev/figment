#pragma once

#include "Figment.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	struct CharacterEditorArgs
	{
		fig::uuid assetId;
		fig::observer_ptr<fig::data::Character> pCharacter;
	};
}
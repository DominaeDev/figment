#pragma once

#include "Figment.h"

namespace fig::data
{
	class World;
}

namespace fig::gui
{
	struct WorldEditorArgs
	{
		fig::uuid assetId;
		fig::observer_ptr<fig::data::World> pWorld;
	};
}
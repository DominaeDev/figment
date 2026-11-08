#pragma once

#include "CoverCard.h"

namespace fig::gui
{
	class WorldCard : public CoverCard
	{
	public:
		WorldCard(ControlPtr pParent, const fig::uuid& worldId, CardSize cardSize);

		void RefreshFull();
		void RefreshMeta();
	};
}

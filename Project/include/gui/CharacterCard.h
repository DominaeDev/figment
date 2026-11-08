#pragma once

#include "CoverCard.h"

namespace fig::gui
{
	class CharacterCard : public CoverCard
	{
	public:
		CharacterCard(ControlPtr pParent, const fig::uuid& characterId, CardSize cardSize);

		void RefreshFull();
		void RefreshMeta();
	};
}

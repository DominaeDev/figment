#ifndef CHARACTER_CARD_H__
#define CHARACTER_CARD_H__

#pragma once

#include "CoverCard.h"

namespace fig::gui
{
	class CharacterCard : public CoverCard
	{
	public:
		CharacterCard(LayoutElement* pParent, const fig::uuid& characterId);

	private:
		fig::uuid _characterId;
	};
}

#endif
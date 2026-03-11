#ifndef CHARACTER_CARD_H__
#define CHARACTER_CARD_H__
#pragma once

#include <set>
#include "CoverCard.h"

namespace fig::gui
{
	class CharacterCard : public CoverCard
	{
	public:
		CharacterCard(LayoutElement* pParent, const fig::uuid& characterId);
		inline const fig::uuid& GetUUID() const noexcept { return _characterId; }

	private:
		fig::uuid _characterId;
		std::unique_ptr<fig::util::Dictionary> _searchWords;
	};
}

#endif
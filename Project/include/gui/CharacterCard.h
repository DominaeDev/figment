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
		CharacterCard(LayoutElement* pParent, const fig::uuid& characterId, CardSize cardSize);
		inline const fig::uuid& GetUUID() const noexcept { return _characterId; }

	protected:
		bool OnEvent(Event& event) override;
		void ShowMenu();

	private:
		fig::uuid _characterId;
		fig::string _characterName;
	};
}

#endif
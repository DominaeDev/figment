#pragma once

#include "CoverCard.h"

namespace fig::gui
{
	class CharacterCard : public CoverCard
	{
	public:
		CharacterCard(LayoutElement* pParent, const fig::uuid& characterId, CardSize cardSize);
		inline const fig::uuid& GetUUID() const noexcept { return _characterId; }

	protected:
		EventResult OnEvent(Event& event) override;
		void ShowMenu();

	private:
		fig::uuid _characterId;
		fig::string _characterName;
		int32_t _menuId { -1 };
	};
}

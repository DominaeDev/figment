#pragma once

#include "CoverCard.h"

namespace fig::gui
{
	class CharacterCard : public CoverCard
	{
	public:
		CharacterCard(ControlPtr pParent, const fig::uuid& characterId, CardSize cardSize);
		
		const fig::uuid& GetCharacterId() const noexcept { return _characterId; }

		void RefreshFull();
		void RefreshMeta();

	protected:
		EventResult OnEvent(fig::event& event) override;
		void ShowMenu();

	private:
		fig::uuid _characterId;
		fig::string _characterName;
		int32_t _menuId { -1 };
	};
}

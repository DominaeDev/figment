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

	protected:
		EventResult OnEvent(fig::event& event) override;
		void ShowMenu();
		void NotifyDelete();

	private:
		fig::string _worldName;
		int32_t _menuId { -1 };
	};
}

#pragma once

#include "CoverCard.h"

namespace fig::gui
{
	class WorldCard : public CoverCard
	{
	public:
		WorldCard(ControlPtr pParent, const fig::uuid& worldId, CardSize cardSize);

		const fig::uuid& GetWorldId() const noexcept { return _worldId; }

		void RefreshFull();
		void RefreshMeta();

	protected:
		EventResult OnEvent(fig::event& event) override;
		void ShowMenu();
		void NotifyDelete();

	private:
		fig::uuid _worldId;
		fig::string _worldName;
		int32_t _menuId { -1 };
	};
}

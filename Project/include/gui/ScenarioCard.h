#pragma once

#include "CoverCard.h"

namespace fig::gui
{
	class ScenarioCard : public CoverCard
	{
	public:
		ScenarioCard(ControlPtr pParent, const fig::uuid& scenarioId, CardSize cardSize);

		void RefreshFull();
		void RefreshMeta();

	protected:
		EventResult OnEvent(fig::event& event) override;
		void ShowMenu();
		void NotifyDelete();

	private:
		fig::string _scenarioName;
		int32_t _menuId { -1 };
	};
}

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
	};
}

#pragma once

#include "gui/CoverCard.h"

namespace fig::gui
{
	class ScenarioCard : public CoverCard
	{
	public:
		ScenarioCard(control_ptr pParent, const fig::uuid& scenarioId, CardSize cardSize);

	protected:
		void OnUpdate(float fElapsed) override {};

	private:
		fig::uuid _scenarioId;
	};
}

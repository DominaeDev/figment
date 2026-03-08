#ifndef SCENARIO_CARD_H__
#define SCENARIO_CARD_H__

#pragma once

#include "gui/CoverCard.h"

namespace fig::gui
{
	class ScenarioCard : public CoverCard
	{
	public:
		ScenarioCard(LayoutElement* pParent, const fig::uuid& scenarioId);

	protected:
		void OnUpdate(float fElapsed) override {};

	private:
		fig::uuid _scenarioId;
	};
}

#endif

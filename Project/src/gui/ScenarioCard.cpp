#include <pch.h>
#include "gui/ScenarioCard.h"
#include "model/AppState.h"
#include "model/UserManager.h"

using namespace fig::fs;

namespace fig::gui
{
	constexpr float Margin = 12.0f;

	ScenarioCard::ScenarioCard(Control* pParent, const fig::uuid& scenarioId) : CoverCard(pParent, scenarioId),
		_scenarioId { scenarioId }
	{
		if (auto scenario = ApplicationState::GetUserManager().GetContent().GetScenario(scenarioId))
		{
			SetLabel(scenario.value().title);
			CreateChatCounter(0);
		}
	}
}
#include <pch.h>
#include "gui/ScenarioCard.h"
#include "app/AppState.h"
#include "user/UserManager.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr float Margin = 12.0f;

	ScenarioCard::ScenarioCard(LayoutElement* pParent, const fig::uuid& scenarioId, CardSize cardSize) : CoverCard(pParent, scenarioId, cardSize),
		_scenarioId { scenarioId }
	{
		if (auto scenario = Global::GetUserManager().GetContent().GetScenario(scenarioId))
		{
			auto [title, desc] = scenario.value().GetInfo();
			SetLabel(title);
			SetChatCount(0);
		}
	}
}
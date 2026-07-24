#include <pch.h>
#include "gui/ScenarioCard.h"
#include "app/AppState.h"
#include "user/UserManager.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::gui
{
	constexpr float Margin = 12.0f;

	ScenarioCard::ScenarioCard(control_ptr pParent, const fig::uuid& scenarioId, CardSize cardSize) : CoverCard(pParent, scenarioId, cardSize),
		_scenarioId { scenarioId }
	{
		if (auto scenario = Global::GetUserContent().Get<Scenario>(scenarioId))
		{
			auto [title, desc] = scenario.value().GetInfo();
			SetLabel(title);
			SetChatCount(_metaData.chatCount);
		}
	}
}
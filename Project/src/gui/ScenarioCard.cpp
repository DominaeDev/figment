#include <pch.h>
#include "gui/ScenarioCard.h"
#include "gui/AppResources.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "gui/Events.h"
#include "gui/Menu.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	ScenarioCard::ScenarioCard(ControlPtr pParent, const fig::uuid& scenarioId, CardSize cardSize) : CoverCard(pParent, scenarioId, cardSize)
	{
		SetDefaultImage(AppResources::GetTexture(Resource::CARD_COVER_STORY));
		RefreshFull();
	}

	void ScenarioCard::RefreshMeta()
	{
		auto& userContent = Global::GetUserContent();
		// Meta data
		if (auto try_meta = userContent.GetMetaData(_assetId))
		{
			auto& meta = *try_meta;
			SetMetaData(meta);
		}
		else // Error
		{
			auto now = fig::now();
			SetMetaData(ContentMetaData {
				.name = _name,
				.createdAt = now,
				.updatedAt = now,
				.lastUsedAt = now,
			});
		}

		// User settings
		SetUserSettings(Global::GetUserContent().GetUserSettings(_assetId));
	}

	void ScenarioCard::RefreshFull()
	{
		if (auto try_scenario = Global::GetUserContent().Get<Scenario>(_assetId); try_scenario.has_value())
		{
			auto& scenario = try_scenario.value();
			_name = scenario.name;
			if (_name.empty())
				_name = scenario.title;
			_updatedAt = Global::GetUserContent().GetUpdatedAt(_assetId).value_or({});
			SetLabel(scenario.name);
//			SetIndex(scenario.GetSearchIndex());

			// Tags
			ClearTags();
			auto& tags = scenario.GetTags();
			for (size_t i = 0; i < tags.size(); ++i)
			{
				if (AddTag(tags[i]) == CoverCard::AddTagResult::Stop)
					break;
			}
		}

		RefreshMeta();
	}

}
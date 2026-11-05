#include <pch.h>
#include "gui/StoryCardList.h"
#include "gui/CardList.impl.h"
#include "gui/ScenarioCard.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	StoryCardList::StoryCardList(ControlPtr pParent, CardSize cardSize) : CardList<fig::data::Scenario>(pParent, cardSize)
	{
	}

	std::tuple<SortBy, OrderBy, FilterFlags> StoryCardList::GetSortOrder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::ScenarioList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::ScenarioList::Ordering, OrderBy::Default);
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, DefaultScenarioFilterFlags, FilterFlagMapping);
		return std::make_tuple(sortBy, orderBy, filterBy);
	}

	template class CardList<fig::data::Scenario>;
}
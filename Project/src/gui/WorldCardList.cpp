#include <pch.h>
#include "gui/WorldCardList.h"
#include "gui/CardList.impl.h"
#include "gui/WorldCard.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	WorldCardList::WorldCardList(ControlPtr pParent, CardSize cardSize) : CardList<fig::data::World>(pParent, cardSize)
	{
	}

	std::tuple<SortBy, OrderBy, FilterFlags> WorldCardList::GetSortOrder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::WorldList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::WorldList::Ordering, OrderBy::Default);
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::WorldList::Filtering, DefaultWorldFilterFlags, FilterFlagMapping);
		return std::make_tuple(sortBy, orderBy, filterBy);
	}

	template class CardList<fig::data::World>;
}
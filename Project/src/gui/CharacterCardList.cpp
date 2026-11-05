#include <pch.h>
#include "gui/CharacterCardList.h"
#include "gui/CardList.impl.h"
#include "gui/CharacterCard.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	CharacterCardList::CharacterCardList(ControlPtr pParent, CardSize cardSize) : CardList<fig::data::Character>(pParent, cardSize)
	{
	}

	std::tuple<SortBy, OrderBy, FilterFlags> CharacterCardList::GetSortOrder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::CharacterList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::CharacterList::Ordering, OrderBy::Default);
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, DefaultCharacterFilterFlags, FilterFlagMapping);
		return std::make_tuple(sortBy, orderBy, filterBy);
	}

	template class CardList<fig::data::Character>;
}
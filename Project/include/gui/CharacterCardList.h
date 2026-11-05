#pragma once

#include "gui/CardList.h"

namespace fig::gui
{
	class CharacterCardList : public CardList<fig::data::Character>
	{
	public:
		CharacterCardList(ControlPtr pParent, CardSize cardSize = CardSize::Full);

	protected:
		std::tuple<fig::io::SortBy, fig::io::OrderBy, fig::io::FilterFlags> GetSortOrder() override;
	};
}

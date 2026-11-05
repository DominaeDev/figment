#pragma once

#include "gui/CardList.h"

namespace fig::gui
{
	class WorldCardList : public CardList<fig::data::World>
	{
	public:
		WorldCardList(ControlPtr pParent, CardSize cardSize = CardSize::Full);

	protected:
		std::tuple<fig::io::SortBy, fig::io::OrderBy, fig::io::FilterFlags> GetSortOrder() override;
	};
}

#pragma once

#include "gui/CardList.h"

namespace fig::gui
{
	class StoryCardList : public CardList<fig::data::Scenario>
	{
	public:
		StoryCardList(ControlPtr pParent, CardSize cardSize = CardSize::Full);

	protected:
		std::tuple<fig::io::SortBy, fig::io::OrderBy, fig::io::FilterFlags> GetSortOrder() override;

		uint32_t OnCardMenu(CardPtr pCard) override;
	};
}

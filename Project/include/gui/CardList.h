#pragma once

#include "gui/ScrollPanel.h"
#include "gui/CoverCard.h"
#include "gui/CardTypes.h"
#include "user/UserSettings.h"

namespace fig::gui
{
	class GridSizer;

	template <typename T>
	class CardList : public ScrollPanel
	{
		using TCard = CardTypeOf<T>::Type;
		using CardPtr = fig::observer_ptr<TCard>;
	public:
		CardList(ControlPtr pParent, CardSize cardSize = CardSize::Full);

		void CreateCards();
		void RefreshCards();
		void Clear();

		void SetFilter(const fig::string& filter) noexcept;
		void Reorder();

		void SetCardSize(CardSize cardSize);
		void EnableTags(bool bEnable) noexcept;
		inline bool IsTagsEnabled() const noexcept { return _bEnableTags; }

	protected:
		virtual std::tuple<fig::io::SortBy, fig::io::OrderBy, fig::io::FilterFlags> GetSortOrder() = 0;

		void OnUpdate(float fElapsed) override;
		void OnScroll() override;
		fig::coord GetExtent() const override;

		void OnCardEvent(CardPtr pCard, CardEvent event);
		void OnDeleteCard(CardPtr pCard);
		
	private:
		bool _bInitialized { false };
		std::vector<CardPtr> _cards;

		fig::observer_ptr<GridSizer> _pGridSizer;
		int32_t _last_rows {};
		CardSize _cardSize;
		fig::string _filterString;
		bool _bEnableTags { false };
	};
}

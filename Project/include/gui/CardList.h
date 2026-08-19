#pragma once

#include "gui/ScrollPanel.h"
#include "gui/CoverCard.h"
#include "user/UserSettings.h"

namespace fig::gui
{
	class GridSizer;

	class CardList : public ScrollPanel
	{
	public:
		CardList(ControlPtr pParent, CardSize cardSize = CardSize::Full);

		enum class CardType { Character, Scenario };
		void CreateCards(CardType cardType);
		void Reset();

		void SetFilter(const fig::string& filter) noexcept;
		void Reorder();

		void SetCardSize(CardSize cardSize);
		void EnableTags(bool bEnable) noexcept;
		inline bool IsTagsEnabled() const noexcept { return _bEnableTags; }

	protected:
		void OnUpdate(float fElapsed) override;
		void OnScroll() override;
		fig::coord GetExtent() const override;

		void OnCardEvent(CoverCard& card, CardEvent event);
		void DeleteCharacter(CoverCard& card);

	private:
		std::vector<fig::observer_ptr<CoverCard>> _cards;

		fig::observer_ptr<GridSizer> _pGridSizer;
		int32_t _last_rows {};
		CardSize _cardSize;
		fig::string _filterString;
		bool _bEnableTags { false };
	};
}

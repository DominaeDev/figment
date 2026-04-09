#ifndef CARD_LIST_H__
#define CARD_LIST_H__
#pragma once

#include "gui/ScrollPanel.h"
#include "gui/CoverCard.h"

namespace fig::gui
{
	class GridSizer;

	class CardList : public ScrollPanel
	{
	public:
		CardList(LayoutElement* pParent, CardSize cardSize = CardSize::Full);

		enum class CardType { Character, Scenario };
		void CreateCards(CardType cardType);
		void Reset();

		void SetFilter(const fig::string&) noexcept;
		void ClearFilter() noexcept;

		void SetCardSize(CardSize cardSize);
		void EnableTags(bool bEnable) noexcept;
		inline bool IsTagsEnabled() const noexcept { return _bEnableTags; }

	protected:
		void OnUpdate(float fElapsed) override;
		void OnScroll() override;

	private:
		std::vector<CoverCard*> _cards;

		GridSizer* _pGridSizer {};
		int32_t _last_rows {};
		CardSize _cardSize;
		bool _bEnableTags { false };
	};
}

#endif
#ifndef CARD_LIST_H__
#define CARD_LIST_H__
#pragma once

#include "gui/ScrollPanel.h"

namespace fig::gui
{
	class GridSizer;
	class CoverCard;

	class CardList : public ScrollPanel
	{
	public:
		CardList(LayoutElement* pParent);

		enum class CardType { Character, Scenario };
		void CreateCards(CardType cardType);

		void SetFilter(const fig::string&) noexcept;
		void ClearFilter() noexcept;

	protected:
		void OnUpdate(float fElapsed) override;

	private:
		std::vector<CoverCard*> _cards;

		GridSizer* _pGridSizer {};
		int32_t _last_rows {};
	};
}

#endif
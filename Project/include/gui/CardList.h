#ifndef CARD_LIST_H__
#define CARD_LIST_H__
#pragma once

#include "gui/ScrollPanel.h"

namespace fig::gui
{
	class GridSizer;

	class CardList : public ScrollPanel
	{
	public:
		CardList(LayoutElement* pParent);

		enum class CardType { Character, Scenario };
		void CreateCards(CardType cardType);

	protected:
		void OnUpdate(float fElapsed) override;

	private:
		GridSizer* _pGridSizer {};
		int32_t _last_rows {};
	};
}

#endif
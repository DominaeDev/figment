#ifndef HOME_FRAME_H__
#define HOME_FRAME_H__
#pragma once

#include "Screen.h"
#include "GUICommon.h"

namespace fig::gui
{
	class CardList;

	class HomeScreen : public Screen
	{
	public:
		HomeScreen(Frame* pParent);

		void CreateCards();

		TYPE_ID(1);
	protected:
		void OnRender(Renderer* pRenderer) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;
		void OnSidePanel(bool show) override;

	private:
		CardList* _pCardList {};
		Control* _pExpandButton {};
	};
}


#endif

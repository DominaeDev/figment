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
		SCREEN_ID(EScreen::Home);
		HomeScreen(Frame* pParent);

		void CreateCards();
		CardList& GetCardList();

	private:
		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;
		void OnSidePanel(bool show) override;
		void OnFilter(fig::string search_text);
		void SetSmallGridSize(bool bSmall);
		void ToggleTags();

	private:
		CardList* _pCardList {};
		Control* _pExpandButton {};
		TextBox* _pFilterTextBox {};
		StaticText* _pHeader {};
		fig::string _search_text;
		float _fSearchTimer {};

	};
}


#endif

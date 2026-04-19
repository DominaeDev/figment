#ifndef HOME_FRAME_H__
#define HOME_FRAME_H__
#pragma once

#include "gui/Screen.h"
#include "gui/GUICommon.h"

namespace fig::gui
{
	class CardList;
	class ToggleWithIcon;

	class HomeScreen : public Screen
	{
	public:
		SCREEN_ID(EScreen::Home);
		HomeScreen(Frame* pParent);

		void CreateCards();
		CardList& GetCardList();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;
		void OnSidePanel(bool show) override;
		void OnSearchFilter(fig::string search_text);
		void OnUserSignedIn(const fig::user::UserProfile& profile) override;

	private:
		void ToggleCardSize() noexcept;
		void ToggleTags() noexcept;
		void ShowSortingMenu() noexcept;
		void ShowFilteringMenu() noexcept;

	private:
		CardList* _pCardList {};
		Control* _pExpandButton {};
		TextBox* _pFilterTextBox {};
		StaticText* _pHeader {};
		fig::string _search_text;
		float _fSearchTimer {};
		ButtonWithIcon* _pSortingButton {};
		ButtonWithIcon* _pFilteringButton {};
		Control* _pFilterBorder {};
		ButtonWithIcon* _pToggleTagsButton {};
		ToggleWithIcon* _pGridButton {};
	};
}


#endif

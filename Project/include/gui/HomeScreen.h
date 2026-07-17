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
		HomeScreen(Frame* pParent);

		void CreateCards();
		CardList& GetCardList();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;
		void OnSearchFilter(fig::string search_text);
		void OnUserSignedIn(const fig::user::UserProfile& profile) override;

	private:
		void ToggleCardSize() noexcept;
		void ToggleTags() noexcept;
		void ShowSortingMenu() noexcept;
		void ShowFilteringMenu() noexcept;

	private:
		fig::observer_ptr<CardList> _pCardList;
		fig::observer_ptr<TextBox> _pFilterTextBox;
		fig::observer_ptr<StaticText> _pHeader;
		fig::observer_ptr<ButtonWithIcon> _pSortingButton;
		fig::observer_ptr<ButtonWithIcon> _pFilteringButton;
		fig::observer_ptr<Control> _pFilterBorder;
		fig::observer_ptr<ButtonWithIcon> _pToggleTagsButton;
		fig::observer_ptr<ToggleWithIcon> _pGridButton;

		fig::string _search_text;
		float _fSearchTimer {};
	};
}

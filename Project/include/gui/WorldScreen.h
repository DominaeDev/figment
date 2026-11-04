#pragma once

#include "gui/Screen.h"
#include "gui/GUICommon.h"

namespace fig::gui
{
	class WorldCardList;
	class ToggleWithIcon;

	class WorldScreen : public Screen
	{
	public:
		WorldScreen(Frame* pParent);

		void RefreshCards();
		WorldCardList& GetCardList();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;
		void OnSearchFilter(fig::string_view search_text);
		void OnUserSignedIn(const fig::user::UserProfile& profile) override;
		void OnActivated() override;

	private:
		void ToggleCardSize() noexcept;
		void ToggleTags() noexcept;
		void ShowSortingMenu() noexcept;
		void ShowFilteringMenu() noexcept;
		void RefreshTopButtons();

	private:
		fig::observer_ptr<WorldCardList> _pCardList;
		fig::observer_ptr<TextInput> _pFilterTextBox;
		fig::observer_ptr<StaticText> _pHeader;
		fig::observer_ptr<ButtonWithIcon> _pSortingButton;
		fig::observer_ptr<ButtonWithIcon> _pFilteringButton;
		fig::observer_ptr<Control> _pFilterBorder;
		fig::observer_ptr<ToggleWithIcon> _pToggleTagsButton;
		fig::observer_ptr<ToggleWithIcon> _pGridButton;

		fig::string _search_text;
		float _fSearchTimer {};
		bool _bRefreshCardList {};
	};

	template <>
	constexpr ScreenType ScreenTypeOf<WorldScreen> = ScreenType::Worlds;
}

#pragma once

#include "gui/Screen.h"
#include "gui/GUICommon.h"

namespace fig::gui
{
	class UserPersonaList;

	class UserPersonaScreen : public Screen
	{
	public:
		UserPersonaScreen(Frame* pParent);

		void Initialize();
	
	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;

		void ShowSortingMenu() noexcept;
		void ShowFilteringMenu() noexcept;
		void OnSearchFilter(fig::string_view search_text);
		void RefreshFilterButton();

	private:
		fig::observer_ptr<UserPersonaList> _pUserPersonaList;
		fig::observer_ptr<ButtonWithIcon> _pSortingButton;
		fig::observer_ptr<ButtonWithIcon> _pFilteringButton;

		fig::string _search_text;
		float _fSearchTimer {};
	};

	template <>
	constexpr ScreenType ScreenTypeOf<UserPersonaScreen> = ScreenType::UserPersonas;
}

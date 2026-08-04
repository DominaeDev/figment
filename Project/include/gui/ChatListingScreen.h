#pragma once

#include "gui/Screen.h"
#include "gui/GUICommon.h"

namespace fig::gui
{
	class ChatList;

	class ChatListingScreen : public Screen
	{
	public:
		ChatListingScreen(Frame* pParent);

		void ShowAllChats();
		void ShowChatsWith(const fig::uuid& characterId);
	
	protected:
		void OnUpdate(float fElapsed) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;
		EventResult OnEvent(fig::event& event) override;
		void ShowSortingMenu() noexcept;
		void ShowFilteringMenu() noexcept;
		void OnSearchFilter(fig::string search_text);
		void RefreshFilterButton();

	private:
		fig::observer_ptr<ChatList> _pChatList;
		fig::observer_ptr<ButtonWithIcon> _pSortingButton;
		fig::observer_ptr<ButtonWithIcon> _pFilteringButton;

		fig::string _search_text;
		float _fSearchTimer {};

		fig::string _filterByCharacter;
	};

	template <>
	constexpr ScreenType ScreenTypeOf<ChatListingScreen> = ScreenType::ChatListing;
}

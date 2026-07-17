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
		bool OnKeyboardEvent(KeyboardEvent& event) override;
		EventResult OnEvent(Event& event) override;
		void ShowSortingMenu() noexcept;
		void ShowFilteringMenu() noexcept;

	private:
		fig::observer_ptr<ChatList> _pChatList;
		fig::observer_ptr<ButtonWithIcon> _pSortingButton;
		fig::observer_ptr<ButtonWithIcon> _pFilteringButton;
	};
}

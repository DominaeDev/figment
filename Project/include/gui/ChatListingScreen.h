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
	
	protected:
		bool OnKeyboardEvent(KeyboardEvent& event) override;
		EventResult OnEvent(Event& event) override;

	private:
		fig::observer_ptr<ChatList> _pChatList;
	};
}

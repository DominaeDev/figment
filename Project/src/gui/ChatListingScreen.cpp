#include <pch.h>
#include "gui/ChatListingScreen.h"
#include "gui/ChatList.h"
#include "app/AppState.h"
#include "user/UserManager.h"

namespace fig::gui
{
	ChatListingScreen::ChatListingScreen(Frame* pParent) : Screen(pParent)
	{
		_pChatList = CreateControl<ChatList>();

		auto mainSizer = new VerticalSizer();
		mainSizer->Add(_pChatList, -1, Sizer::Fill | Sizer::Left, 12);
		SetSizer(mainSizer);

		InvalidateLayout();
	}

	void ChatListingScreen::ShowAllChats()
	{
		if (not Global::IsSignedIn())
			return;

		_pChatList->ShowAllChats();
	}

	bool ChatListingScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		return false;
	}

	EventResult ChatListingScreen::OnEvent(Event& event)
	{
		if (IsUserEvent(event, UserEvent::Activated))
		{
			ShowAllChats();
		}

		return Screen::OnEvent(event);
	}

}
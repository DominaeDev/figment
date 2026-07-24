#include <pch.h>
#include "gui/Frame.h"
#include "gui/Screen.h"

namespace fig::gui
{
	Screen::Screen(Frame* pParent) : Control(pParent)
	{
		SetSize(pParent->GetSize());
		SetForegroundColor(Color::Black);
		SetBackgroundColor(Color::AppBackground);
	}

	EventResult Screen::OnEvent(fig::event& event)
	{
		// Key press or release
		if ((event.type == SDL_EVENT_KEY_DOWN or event.type == SDL_EVENT_KEY_UP) and not event.key.repeat)
		{
			KeyboardEvent keyEvent {
				.key = event.key.key,
				.modifiers = KeyboardMods { event },
				.pressed = event.key.down,
			};

			if (OnKeyboardEvent(keyEvent))
				return EventResult::Handled;
		}

		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			return EventResult::Pass;
		}

		if (IsUserEvent(event, UserEvent::UserSignedIn))
		{
			OnUserSignedIn(GetUserData<fig::user::UserProfile>(event));
			return EventResult::Continue;
		}

		if (IsUserEvent(event, UserEvent::UserSignedOut))
		{
			OnUserSignedOut();
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}

	void Screen::PushEvent(UserEvent eventType, int32_t code, void* pData1, void* pData2)
	{
		fig::event event {};
		event.type = SDLUserEvent(eventType);
		event.user.code = code;
		event.user.data1 = pData1;
		event.user.data2 = pData2;
		ProcessEvent(event);
	}	
}
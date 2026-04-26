#include <pch.h>
#include "gui/Frame.h"
#include "gui/Screen.h"

namespace fig::gui
{
	Screen::Screen(Frame* pParent) : Control(pParent)
	{
		SetSize(pParent->GetSize());
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);
	}

	bool Screen::OnEvent(Event& event)
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
				return true;
		}

		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			return false;
		}

		if (event.type == SDLUserEvent(EventType::UserSignedIn))
		{
			OnUserSignedIn(*reinterpret_cast<const fig::user::UserProfile*>(event.user.data1));
		}

		if (event.type == SDLUserEvent(EventType::UserSignedIn))
		{
			OnUserSignedOut();
		}

		return false;
	}

	void Screen::NotifySidePanelShown(bool bShow)
	{
		OnSidePanel(bShow);
	}
}
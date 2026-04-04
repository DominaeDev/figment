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
			bool bCtrlDown = (event.key.mod & SDL_KMOD_CTRL) != 0;
			bool bShiftDown = (event.key.mod & SDL_KMOD_SHIFT) != 0;
			bool bAltDown = (event.key.mod & SDL_KMOD_ALT) != 0;

			KeyModifiers mods = {
				bCtrlDown ? KeyModifier::Control : KeyModifier::None,
				bShiftDown ? KeyModifier::Shift : KeyModifier::None,
				bAltDown ? KeyModifier::Alt : KeyModifier::None,
			};
			
			KeyboardEvent keyEvent {
				.key = event.key.key,
				.modifiers = mods,
				.pressed = event.key.down,
			};

			if (OnKeyboardEvent(keyEvent))
				return true;
		}

		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			return false;
		}

		return false;
	}

	void Screen::NotifySidePanelShown(bool bShow)
	{
		OnSidePanel(bShow);
	}
}
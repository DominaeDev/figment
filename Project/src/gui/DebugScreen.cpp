#include <pch.h>
#include "gui/DebugScreen.h"

namespace fig::gui
{
	DebugScreen::DebugScreen(Frame* pParent) : Screen(pParent)
	{
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		auto mainSizer = new VerticalSizer();
		SetSizer(mainSizer);
	}

	void DebugScreen::OnUpdate(float fElapsed)
	{
	}

	void DebugScreen::OnRender(Renderer* pRenderer)
	{
	}

	bool DebugScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed) // Press
		{
		}
		else // Release
		{
		}
		return false;
	}

}
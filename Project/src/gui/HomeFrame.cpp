#include <pch.h>
#include "gui/HomeFrame.h"

namespace fig::gui
{
	HomeFrame::HomeFrame(Frame* pParent) : Screen(pParent)
	{
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		auto mainArea = new Area(this);

		auto topSizer = new VerticalSizer();
		topSizer->Add(mainArea, -1, Sizer::Expand);
		SetSizer(topSizer);

		InvalidateLayout();
	}

	void HomeFrame::OnUpdate(float fDeltaTime)
	{

	}

	void HomeFrame::OnRender(Renderer* pRenderer)
	{
		DrawBackground(pRenderer);
	}

	bool HomeFrame::OnKeyboardEvent(KeyboardEvent& event)
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
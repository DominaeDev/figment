#include <pch.h>
#include "gui/DebugScreen.h"

namespace fig::gui
{
	DebugScreen::DebugScreen(Frame* pParent) : Screen(pParent)
	{
		auto area = new Panel(this);
		area->SetBackgroundColor(Colors::Black);
		area->SetPosition(200, 200);
		area->SetSize(500, 200);

		auto left = new Panel(area);
		left->SetBackgroundColor(Color { 0xC0, 0, 0, 0xFF });
		left->SetSize(100, 100);

		auto center = new Panel(area);
		center->SetBackgroundColor(Color { 0, 0xC0, 0, 0xFF });
		center->SetSize(100, 100);

		auto right = new Panel(area);
		right->SetBackgroundColor(Color { 0, 0, 0xC0, 0xFF });
		right->SetSize(100, 100);

		auto sizer = new HorizontalSizer();
		sizer->Add(left, 0);
		sizer->Add(center, -1);
		sizer->Add(right, 0, Sizer::Expand | Sizer::All, 10);

		area->SetSizer(sizer);
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
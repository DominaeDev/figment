#include <pch.h>
#include "gui/DebugScreen.h"
#include "gui/CardImage.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithLabel.h"

namespace fig::gui
{
	DebugScreen::DebugScreen(Frame* pParent) : Screen(pParent)
	{
		auto area = new Panel(this);
		area->SetBackgroundColor(Colors::Black);
		area->SetPosition(200, 200);
		area->SetSize(500, 500);

		auto left = new Panel(area);
		left->SetBackgroundColor(Color { 0xC0, 0, 0, 0xFF });
		left->SetSize(50, 50);

		auto center = new Panel(area);
		center->SetBackgroundColor(Color { 0, 0xC0, 0, 0xFF });
		center->SetSize(100, 100);

		auto right = new Area(area);
		right->SetBackgroundColor(Color { 0, 0, 0xC0, 0xFF });
		right->SetSize(100, 100);
		auto h = new HorizontalSizer();
		right->SetSizer(h);

		auto a = new Panel(right);
		a->SetBackgroundColor(Colors::Red);
		a->SetSize(50, 50);
		auto b = new Panel(right);
		b->SetBackgroundColor(Colors::Green);
		b->SetSize(50, 50);
		auto c = new Panel(right);
		c->SetBackgroundColor(Colors::Blue);
		c->SetSize(50, 50);

		h->Add(a, -1, Sizer::AlignRight | Sizer::AlignTop, 4);
		h->Add(b, 0, Sizer::AlignCenterHorizontal | Sizer::AlignCenterVertical, 4);
		h->Add(c, -1, Sizer::AlignLeft | Sizer::AlignBottom, 4);

		auto sizer = new VerticalSizer();
		sizer->Add(left, -1, Sizer::AlignRight | Sizer::AlignCenterVertical);
		sizer->AddSpacer(4);
		sizer->Add(center, -1, Sizer::Fill);
		sizer->AddSpacer(4);
		sizer->Add(right, 0, Sizer::Expand | Sizer::AlignCenterVertical | Sizer::FixedSize, 100);

		auto pButton = new ButtonWithLabel(this, "Invalidate");
		pButton->SetHeight(35);
		pButton->SetDelegate([this]() { InvalidateLayout(); });

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
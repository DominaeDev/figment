#include <pch.h>
#include "gui/DebugScreen.h"
#include "gui/CardImage.h"
#include "gui/AppResources.h"
#include "gui/ButtonWithLabel.h"
#include "gui/Slider.h"
#include "gui/ButtonWithLabelAndIcon.h"

namespace fig::gui
{
	DebugScreen::DebugScreen(Frame* pParent) : Screen(pParent)
	{
		auto area = CreateControl<Panel>();
		area->SetBackgroundColor(Color::Black);
		area->SetPosition(200, 200);
		area->SetSize(500, 500);

		auto left = area->CreateControl<Panel>();
		left->SetBackgroundColor(fig::color { 0xC0, 0, 0, 0xFF });
		left->SetSize(50, 50);

		auto center = area->CreateControl<Panel>();
		center->SetBackgroundColor(fig::color { 0, 0xC0, 0, 0xFF });
		center->SetSize(100, 100);

		auto right = area->CreateControl<Panel>();
		right->SetBackgroundColor(fig::color { 0, 0, 0xC0, 0xFF });
		right->SetSize(100, 100);

		auto h = right->SetSizer<HorizontalSizer>();

		auto a = right->CreateControl<Panel>();
		a->SetBackgroundColor(Color::Red);
		a->SetSize(50, 50);
		auto b = right->CreateControl<Panel>();
		b->SetBackgroundColor(Color::Green);
		b->SetSize(50, 50);
		auto c = right->CreateControl<Panel>();
		c->SetBackgroundColor(Color::Blue);
		c->SetSize(50, 50);

		h->Add(a, -1, SizerFlag::AlignRight | SizerFlag::AlignTop, 4);
		h->Add(b, 0, SizerFlag::AlignCenterHorizontal | SizerFlag::AlignCenterVertical, 4);
		h->Add(c, -1, SizerFlag::AlignLeft | SizerFlag::AlignBottom, 4);

		auto sizer = area->SetSizer<VerticalSizer>();
		sizer->Add(left, -1, SizerFlag::AlignRight | SizerFlag::AlignCenterVertical);
		sizer->AddSpacer(4);
		sizer->Add(center, -1, SizerFlag::Fill);
		sizer->AddSpacer(4);
		sizer->Add(right, 0, SizerFlag::Expand | SizerFlag::AlignCenterVertical | SizerFlag::FixedSize, 100);

		auto pButton = CreateControl<ButtonWithLabel>("Invalidate");
		pButton->SetHeight(35);
		pButton->SetDelegate([this]() { InvalidateLayout(); });

		auto pSlider = CreateControl<Slider>();
		pSlider->SetPosition(50, 200);

		auto pButton2 = CreateControl<ButtonWithLabelAndIcon>("Save", Resource::ICON_SAVE);

		pButton2->SetPosition(400, 50);
	}

	void DebugScreen::OnUpdate(float fElapsed)
	{
	}

	void DebugScreen::OnRender(fig::renderer_ptr pRenderer)
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
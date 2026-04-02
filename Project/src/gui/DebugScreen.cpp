#include <pch.h>
#include "gui/DebugScreen.h"
#include "gui/CardImage.h"
#include "gui/AppResources.h"

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
		left->SetSize(50, 50);

		auto center = new Panel(area);
		center->SetBackgroundColor(Color { 0, 0xC0, 0, 0xFF });
		center->SetSize(100, 100);

		auto right = new Panel(area);
		right->SetBackgroundColor(Color { 0, 0, 0xC0, 0xFF });
		right->SetSize(100, 100);

		auto sizer = new HorizontalSizer();
		sizer->Add(left, -1, Sizer::AlignRight | Sizer::AlignCenterVertical);
		sizer->Add(center, -1, Sizer::Fill);
		sizer->Add(right, 0, Sizer::Expand | Sizer::All, 10);

		area->SetSizer(sizer);

		auto pCard = new CardImage(this, AppResources::GetTexture(TextureType::CARD_BACKGROUND_EMPTY), AppResources::GetTexture(TextureType::MASK_CARD));
		pCard->SetSize(Constants::GUI::CardWidth, Constants::GUI::CardHeight);
		pCard->SetPosition(500, 300);

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
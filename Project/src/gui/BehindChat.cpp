#include <pch.h>
#include "gui/BehindChat.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr fig::coord GradientWidth = 80;

	BehindChat::BehindChat(ControlPtr pParent) : Area(pParent)
	{
		_pBG = CreateControl<Panel>();
		_pLeftGradient = CreateControl<Image>(Resource::MASK_GRADIENT_EASE_IN_CUBIC_LEFT);
		_pLeftGradient->SetWidth(GradientWidth);
		_pRightGradient = CreateControl<Image>(Resource::MASK_GRADIENT_EASE_IN_CUBIC_RIGHT);
		_pRightGradient->SetWidth(GradientWidth);

		auto pSizer = SetSizer<HorizontalSizer>();
		pSizer->Add(_pLeftGradient, 0, SizerFlag::Expand);
		pSizer->Add(_pBG, -1, SizerFlag::Fill);
		pSizer->Add(_pRightGradient, 0, SizerFlag::Expand);

		SetColor(Color::Black);
	}

	void BehindChat::SetColor(fig::color color)
	{
		SetBackgroundColor(color);

		_pBG->SetBackgroundColor(color);
		_pLeftGradient->SetForegroundColor(color);
		_pRightGradient->SetForegroundColor(color);

		SetVisible(color.a);
	}

	void BehindChat::SetColor(fig::color color, float fAlpha)
	{
		SetBackgroundColor(color.WithAlpha(fAlpha));
	}

	void BehindChat::SetAlpha(float fAlpha)
	{
		SetColor(GetBackgroundColor().WithAlpha(fAlpha));
	}
}
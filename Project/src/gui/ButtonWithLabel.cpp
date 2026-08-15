#include <pch.h>
#include "gui/ButtonWithLabel.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"

namespace fig::gui
{
	ButtonWithLabel::ButtonWithLabel(ControlPtr pParent, fig::string_view text, double fontSize) : ThemedButton(pParent)
	{
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundColor(GetThemeBackground());

		auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorderRenderer->SetColor(Color::LineColor);

		_pLabel = CreateControl<StaticText>("", FontFace::Default, fontSize, true);
		_pLabel->SetForegroundColor(GetThemeForeground());
		_pLabel->SetTextAndResize(text);

		SetSize(200, 36);
	}

	void ButtonWithLabel::SetLabel(fig::string_view label) noexcept
	{
		_pLabel->SetTextAndResize(label);
		_pLabel->Center();
	}

	void ButtonWithLabel::OnSize()
	{
		_pLabel->Center();
	}

	void ButtonWithLabel::OnButtonState()
	{
		GetBackgroundRenderer()->SetColor(GetThemeBackground());
		GetBorderRenderer()->SetColor(_state != ButtonState::Disabled ? Color::LineColor : Color::DisabledLineColor);

		_pLabel->SetForegroundColor(GetThemeForeground());
	}
}
#include <pch.h>
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"

namespace fig::gui
{
	ButtonWithLabelAndIcon::ButtonWithLabelAndIcon(ControlPtr pParent, fig::string_view text, Resource icon, double fontSize) : ThemedButton(pParent)
	{
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundColor(GetThemeBackground());

		auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorderRenderer->SetColor(Color::LineColor);

		_pLabel = CreateControl<StaticText>("", FontFace::Default, fontSize, true);
		_pLabel->SetForegroundColor(GetThemeForeground());
		_pLabel->SetTextAndResize(text);

		_pIcon = CreateControl<Image>(AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());

		SetSize(200, 36);
	}

	void ButtonWithLabelAndIcon::SetLabel(fig::string_view label) noexcept
	{
		_pLabel->SetTextAndResize(label);
		OnSize();
	}

	void ButtonWithLabelAndIcon::SetIcon(Resource icon)
	{
		_pIcon->SetTexture(AppResources::GetTexture(icon));
	}

	void ButtonWithLabelAndIcon::OnSize()
	{
		constexpr fig::coord kIconRegionWidth = 40;
		constexpr fig::coord kMargin = 6;

		_pIcon->SetX((kIconRegionWidth - _pIcon->GetWidth()) / 2);
		_pIcon->CenterVertically();
		_pLabel->SetX(kIconRegionWidth + ((GetWidth() - kIconRegionWidth - kMargin * 2) - _pLabel->GetWidth()) / 2);
		_pLabel->CenterVertically();
	}

	void ButtonWithLabelAndIcon::OnButtonState()
	{
		GetBackgroundRenderer()->SetColor(GetThemeBackground());
		GetBorderRenderer()->SetColor(_state != ButtonState::Disabled ? Color::LineColor : Color::DisabledLineColor);

		_pLabel->SetForegroundColor(GetThemeForeground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}
}
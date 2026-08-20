#include <pch.h>
#include "gui/ButtonWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	ButtonWithIcon::ButtonWithIcon(ControlPtr pParent, Resource icon, bool bBorder) : ThemedButton(pParent)
	{
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBGRenderer->SetColor(GetThemeBackground());

		if (bBorder)
		{
			auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
			pBorderRenderer->SetColor(Color::LineColor);
		}

		_pIcon = CreateControl<Image>(AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());

		SetSize(36, 36);
	}

	void ButtonWithIcon::SetIcon(Resource icon)
	{
		_pIcon->SetTexture(AppResources::GetTexture(icon));
	}

	void ButtonWithIcon::OnSize()
	{
		if (_pIcon)
			_pIcon->Center();
	}

	void ButtonWithIcon::OnButtonState()
	{
		GetBackgroundRenderer()->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
		
		ShowBorder(_bShowBorder);
	}

	void ButtonWithIcon::ShowBorder(bool bShow) noexcept
	{
		if (auto pBorder = GetBorderRenderer())
			pBorder->SetColor(bShow ? (GetEnabled() ? Color::LineColor : Color::DisabledLineColor) : Color::Transparent);
		_bShowBorder = bShow;
	}
}
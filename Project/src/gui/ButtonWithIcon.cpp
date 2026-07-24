#include <pch.h>
#include "gui/ButtonWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	ButtonWithIcon::ButtonWithIcon(control_ptr pParent, Resource icon) : ThemedButton(pParent)
	{
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBGRenderer->SetColor(GetThemeBackground());

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
	}

	void ButtonWithIcon::EnableBorder(bool bEnable) noexcept
	{
		if (bEnable)
		{
			auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
			pBorder->SetColor(Color::LineColor);
		}
		else
		{
			ClearBorderRenderer();
		}
	}
}
#include <pch.h>
#include "gui/ButtonWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	ButtonWithIcon::ButtonWithIcon(ControlPtr pParent, TextureResource icon, bool bBorder) : ThemedButton(pParent)
	{
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBGRenderer->SetColor(GetThemeBackground());

		if (bBorder)
		{
			auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
			pBorderRenderer->SetColor(Color::Border);
		}

		_pIcon = CreateControl<Image>(icon);
		_pIcon->SetForegroundColor(GetThemeForeground());

		SetSize(36, 36);
	}

	void ButtonWithIcon::SetIcon(TextureResource icon)
	{
		_pIcon->SetTexture(*icon);
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

}
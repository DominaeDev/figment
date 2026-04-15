#include <pch.h>
#include "gui/ButtonWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	ButtonWithIcon::ButtonWithIcon(LayoutElement* pParent, TextureType icon) : ThemedButton(pParent)
	{
		_pBGRenderer = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		_pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundRenderer(_pBGRenderer);

		_pIcon = new Image(this, AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());

		SetSize(36, 36);
	}

	void ButtonWithIcon::OnSize()
	{
		if (_pIcon)
			_pIcon->Center();
	}

	void ButtonWithIcon::OnButtonState()
	{
		_pBGRenderer->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}

	void ButtonWithIcon::EnableBorder(bool bEnable) noexcept
	{
		if (bEnable)
		{
			_pBorder = new TexturedBorderRenderer(TextureType::ROUNDED_BORDER_6PX, 8);
			_pBorder->SetColor(Colors::LineColor);
			SetBorderRenderer(_pBorder);
		}
		else
		{
			SetBorderRenderer(nullptr);
			_pBorder = nullptr;
		}
	}
}
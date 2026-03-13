#include <pch.h>
#include "gui/ButtonWithIcon.h"
#include "gui/NineGridRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	ButtonWithIcon::ButtonWithIcon(LayoutElement* pParent, TextureType icon) : ThemedButton(pParent)
	{
		_pBGRenderer = new NineGridRenderer(8);
		_pBGRenderer->SetTexture(AppResources::GetTexture(TextureType::ROUNDED_BACKGROUND));
		_pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundRenderer(_pBGRenderer);

		_pIcon = new Image(this, AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());
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
}
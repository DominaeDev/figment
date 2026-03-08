#include <pch.h>
#include "gui/IconButton.h"
#include "gui/NineGridBackgroundRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	IconButton::IconButton(LayoutElement* pParent, TextureType icon) : ThemedButton(pParent)
	{
		_pFaceRenderer = new NineGridBackgroundRenderer(8);
		_pFaceRenderer->SetTexture(AppResources::GetTexture(TextureType::BUTTON_BACKGROUND));
		_pFaceRenderer->SetColor(GetThemeBackground());
		SetBackgroundRenderer(_pFaceRenderer);

		_pIcon = new Image(this, AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());
	}

	void IconButton::OnSize()
	{
		if (_pIcon)
			_pIcon->Center();
	}

	void IconButton::OnButtonState()
	{
		_pFaceRenderer->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}
}
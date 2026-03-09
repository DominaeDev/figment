#include <pch.h>
#include "gui/SidePanelButton.h"
#include "gui/NineGridRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	SidePanelButton::SidePanelButton(LayoutElement* pParent, TextureType icon, const fig::string& label) : ThemedButton(pParent)
	{
		_pBGRenderer = new NineGridRenderer(8);
		_pBGRenderer->SetTexture(AppResources::GetTexture(TextureType::BUTTON_BACKGROUND));
		_pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundRenderer(_pBGRenderer);

		_pIcon = new Image(this, AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());

		_pLabel = new StaticText(this, label, FontFace::Default, 20, false);
		_pLabel->SetForegroundColor(GetThemeForeground());
		_pLabel->SetBackgroundColor(Colors::Transparent);
		_pLabel->EnableEllipsis(true);
		_pLabel->SetPosition(60, 16);
	}

	void SidePanelButton::OnSize()
	{
		if (_pIcon)
		{
			_pIcon->SetX(4);
			_pIcon->CenterVertically();
		}

		_pLabel->SetMaxSize(GetWidth() - _pLabel->GetX() - 8, -1);
	}

	void SidePanelButton::OnButtonState()
	{
		_pBGRenderer->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}
}
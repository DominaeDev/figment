#include <pch.h>
#include "gui/SidePanelButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	SidePanelButton::SidePanelButton(LayoutElement* pParent, TextureType icon, const fig::string& label) : ThemedButton(pParent)
	{
		auto pBorder = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		pBorder->SetColor(GetThemeBackground());
		SetBackgroundRenderer(pBorder);
		_pBorder = pBorder;

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
		_pBorder->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}
}
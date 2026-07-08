#include <pch.h>
#include "gui/SidePanelButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	SidePanelButton::SidePanelButton(ParentPtr pParent, TextureType icon, const fig::string& label) : ThemedButton(pParent)
	{
		SetTheme(Themes::SidePanelButtonStyle);
		SetHeight(58);

		auto pBorder = SetBackgroundRenderer<TexturedBorderRenderer>(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		pBorder->SetColor(GetThemeBackground());
		_pBorder = pBorder;

		_pIcon = CreateControl<Image>(AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());

		_pLabel = CreateControl<StaticText>(label, FontFace::Default, 20, false);
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

		if (_pLabel)
			_pLabel->SetMaxSize(GetWidth() - _pLabel->GetX() - 8, -1);
	}

	void SidePanelButton::OnButtonState()
	{
		_pBorder->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}
}
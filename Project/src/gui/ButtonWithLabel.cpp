#include <pch.h>
#include "gui/ButtonWithLabel.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"

namespace fig::gui
{
	ButtonWithLabel::ButtonWithLabel(LayoutElement* pParent, const fig::string& text) : ThemedButton(pParent)
	{
		SetSize(200, 36);

		_pBGRenderer = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		_pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundRenderer(_pBGRenderer);
		SetBackgroundColor(GetThemeBackground());

		_pLabel = CreateControl<StaticText>("", FontFace::Default, 16.0, true);
		_pLabel->SetForegroundColor(GetThemeForeground());
		_pLabel->SetTextAndResize(text);
		_pLabel->Center();

		_pBorder = CreateControl<TexturedBorder>(AppResources::GetTexture(TextureType::CARD_BORDER), 16);
		_pBorder->SetForegroundColor(Colors::SidePanelForeground);
		_pBorder->FillParent();

	}

	void ButtonWithLabel::OnAfterLayout()
	{
		if (_pBorder)
			_pBorder->FillParent();
	}

	void ButtonWithLabel::OnButtonState()
	{
		_pBGRenderer->SetColor(GetThemeBackground());
		_pLabel->SetForegroundColor(GetThemeForeground());
	}
}
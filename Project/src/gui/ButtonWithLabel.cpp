#include <pch.h>
#include "gui/ButtonWithLabel.h"
#include "gui/NineGridRenderer.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"

namespace fig::gui
{
	ButtonWithLabel::ButtonWithLabel(LayoutElement* pParent, const fig::string& text) : ThemedButton(pParent)
	{
		SetSize(200, 36);

		_pBGRenderer = new NineGridRenderer(8);
		_pBGRenderer->SetTexture(AppResources::GetTexture(TextureType::ROUNDED_BACKGROUND));
		_pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundRenderer(_pBGRenderer);
		SetBackgroundColor(GetThemeBackground());

		_pLabel = new StaticText(this, "", FontFace::Default, 16.0, true);
		_pLabel->SetForegroundColor(GetThemeForeground());
		_pLabel->SetTextAndResize(text);
		_pLabel->Center();

		_pBorder = new TexturedBorder(this, AppResources::GetTexture(TextureType::CARD_BORDER), 16);
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
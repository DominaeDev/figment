#include <pch.h>
#include "gui/ButtonWithLabel.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"

namespace fig::gui
{
	ButtonWithLabel::ButtonWithLabel(control_ptr pParent, const fig::string& text) : ThemedButton(pParent)
	{
		SetSize(200, 36);

		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundColor(GetThemeBackground());

		_pLabel = CreateControl<StaticText>("", FontFace::Default, 16.0, true);
		_pLabel->SetForegroundColor(GetThemeForeground());
		_pLabel->SetTextAndResize(text);
		_pLabel->Center();

		_pBorder = CreateControl<TexturedBorder>(AppResources::GetTexture(Resource::CARD_BORDER), 16);
		_pBorder->SetForegroundColor(Color::SidePanelForeground);
		_pBorder->FillParent();

	}

	void ButtonWithLabel::OnAfterLayout()
	{
		if (_pBorder)
			_pBorder->FillParent();
	}

	void ButtonWithLabel::OnButtonState()
	{
		GetBackgroundRenderer()->SetColor(GetThemeBackground());
		_pLabel->SetForegroundColor(GetThemeForeground());
	}
}
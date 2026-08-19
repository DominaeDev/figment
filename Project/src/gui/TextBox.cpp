#include <pch.h>
#include "gui/TextBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"

namespace fig::gui
{
	TextBox::TextBox(ControlPtr pParent, FontFace fontFace, double ptSize, Flags flags) : TextInput(pParent, fontFace, ptSize, flags)
	{
		SetMargins(8, 4, 4, 6);

		if (_pFont)
			SetSize(300, MeasureFontHeight(*_pFont) + GetMarginVertical());
				
		auto pTextBoxBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Color::TextBoxBackground);

		auto pTextBoxBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color::LineColor);
	}

	void TextBox::OnEnabled(bool bEnabled)
	{
		TextInput::OnEnabled(bEnabled);
		GetBackgroundRenderer()->SetColor(bEnabled ? Color::White : Color::DisabledBackground);
	}

}
#include <pch.h>
#include "gui/SimpleTextBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"

namespace fig::gui
{
	SimpleTextBox::SimpleTextBox(ParentPtr pParent, FontFace fontFace, double ptSize, Flags flags) : TextBox(pParent, fontFace, ptSize, flags)
	{
		if (_pFont)
			SetSize(300, MeasureFontHeight(*_pFont) + GetMarginVertical());
		
		auto pTextBoxBG = SetBackgroundRenderer<TexturedBorderRenderer>(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Colors::White);

		auto pTextBoxBorder = SetBorderRenderer<TexturedBorderRenderer>(TextureType::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color { 0x61, 0x5a, 0x35, 0xFF });
	}

	void SimpleTextBox::OnEnabled(bool bEnabled)
	{
		TextBox::OnEnabled(bEnabled);
		GetBackgroundRenderer()->SetColor(bEnabled ? Colors::White : Colors::DisabledBackground);
	}
}
#include <pch.h>
#include "gui/SimpleTextBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"

namespace fig::gui
{
	SimpleTextBox::SimpleTextBox(LayoutElement* pParent, FontFace fontFace, double ptSize, Flags flags) : TextBox(pParent, fontFace, ptSize, flags)
	{
		if (_pFont)
			SetSize(300, MeasureFontHeight(*_pFont) + GetMarginVertical());
		
		_pTextBoxBG = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		_pTextBoxBG->SetExtend(0.0f);
		_pTextBoxBG->SetColor(Colors::White);
		SetBackgroundRenderer(_pTextBoxBG);

		auto pTextBoxBorder = new TexturedBorderRenderer(TextureType::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color { 0x61, 0x5a, 0x35, 0xFF });
		SetBorderRenderer(pTextBoxBorder);
	}

	void SimpleTextBox::OnEnabled(bool bEnabled)
	{
		TextBox::OnEnabled(bEnabled);
		_pTextBoxBG->SetColor(bEnabled ? Colors::White : Colors::DisabledBackground);
	}
}
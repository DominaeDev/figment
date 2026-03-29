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
		
		auto pTextBoxBG = new NineGridRenderer(8);
		pTextBoxBG->SetCornerSize(20.0f);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Colors::White);
		pTextBoxBG->SetTexture(AppResources::GetTexture(TextureType::ROUNDED_BACKGROUND));
		SetBackgroundRenderer(pTextBoxBG);

		auto pTextBoxBorder = new NineGridRenderer(8);
		pTextBoxBorder->SetCornerSize(20.0f);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color { 0x61, 0x5a, 0x35, 0xFF });
		pTextBoxBorder->SetTexture(AppResources::GetTexture(TextureType::ROUNDED_BORDER));
		SetBorderRenderer(pTextBoxBorder);
	}
}
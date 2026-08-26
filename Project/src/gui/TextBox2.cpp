#include <pch.h>
#include "gui/TextBox2.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"

namespace fig::gui
{
	TextBox2::TextBox2(ControlPtr pParent, FontFace fontFace, double ptSize, Flags flags) : TextInput2(pParent, fontFace, ptSize, flags)
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

	void TextBox2::OnEnabled(bool bEnabled)
	{
		TextInput2::OnEnabled(bEnabled);
		GetBackgroundRenderer()->SetColor(bEnabled ? Color::White : Color::DisabledBackground);
		GetBorderRenderer()->SetColor(bEnabled ? Color::LineColor : Color::DisabledLineColor);
	}

	void TextBox2::SetFixedRows(int32_t rows)
	{
		if (_pFont)
			SetHeight(TTF_GetFontLineSkip(_pFont) * rows + GetMarginVertical());
		_minRows = rows;
		_maxRows = rows;
		_flags.Unset(Flag::Autosize);
	}

}
#include <pch.h>
#include "gui/TextBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"
#include "gui/VerticalBar.h"

namespace fig::gui
{
	static constexpr fig::coord kScrollBarMarginY = 4;
	static constexpr fig::coord kScrollBarRight = 7;

	TextBox::TextBox(ControlPtr pParent, FontFace fontFace, double ptSize, TextInput::Mode mode) : TextInput(pParent, fontFace, ptSize, mode)
	{
		SetMargins(8, 4, 6, 6);

		if (_pFont)
			SetSize(300, MeasureFontHeight(*_pFont) + GetMarginVertical());
				
		auto pTextBoxBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Color::TextBoxBackground);

		auto pTextBoxBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color::LineColor);

		_pScrollBar = CreateControl<VerticalBar>(Resource::VERTICAL_BAR_SMALL);
		_pScrollBar->SetForegroundColor(0x00000060_rgba);
		_pScrollBar->SetX(GetWidth() - kScrollBarRight);
		_pScrollBar->SetSize(3, 16);
	}

	void TextBox::OnEnabled(bool bEnabled)
	{
		TextInput::OnEnabled(bEnabled);
		GetBackgroundRenderer()->SetColor(bEnabled ? Color::White : Color::DisabledBackground);
		GetBorderRenderer()->SetColor(bEnabled ? Color::LineColor : Color::DisabledLineColor);
	}

	void TextBox::SetFixedRows(int32_t rows)
	{
		if (_pFont)
			SetHeight(TTF_GetFontLineSkip(_pFont) * rows + GetMarginVertical());
		_minRows = rows;
		_maxRows = rows;
		_bAutoSize = false;
	}

	void TextBox::OnUpdate(float fElapsed)
	{
		TextInput::OnUpdate(fElapsed);
		RefreshScrollBar();
	}

	void TextBox::OnSize()
	{
		TextInput::OnSize();
		RefreshScrollBar();
	}

	void TextBox::RefreshScrollBar()
	{
		if (not (bool)_pScrollBar)
			return;

		if (_lines.size() <= 1uz)
		{
			if (_pScrollBar->GetVisible())
				_pScrollBar->SetVisible(false);
			return;
		}

		auto text = GetText();

		float fExtent = toF(GetLineCount() * _lineHeight);

		int32_t pageHeight = GetClientRect().h;
		float fScrollRange = fExtent - pageHeight;
		if (fScrollRange <= 0.0f)
		{
			if (_pScrollBar->GetVisible())
				_pScrollBar->SetVisible(false);
			return;
		}

		if (not _pScrollBar->GetVisible())
			_pScrollBar->SetVisible(true);

		int32_t handleSize = std::clamp(toI(std::min(toF(pageHeight) / fExtent, 1.0f) * pageHeight), 20, std::max(pageHeight, 16));
		_pScrollBar->SetY(kScrollBarMarginY + toI((_scroll.y / fScrollRange) * (GetHeight() - handleSize - kScrollBarMarginY * 2)));
		_pScrollBar->SetX(GetWidth() - kScrollBarRight);
		_pScrollBar->SetHeight(handleSize);
	}
}
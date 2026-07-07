#include <pch.h>
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	SearchBox::SearchBox(LayoutElement* pParent, FontFace fontFace, double ptSize) : TextBox(pParent, fontFace, ptSize)
	{
		if (_pFont)
			SetSize(300, MeasureFontHeight(*_pFont) + GetMarginVertical());
		
		SetMarginLeft(30);

		_pIcon = CreateControl<Image>(AppResources::GetTexture(TextureType::ICON_SEARCH));
		_pIcon->SetForegroundColor(Colors::SidePanelForeground);

		auto pTextBoxBG = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Colors::White);
		SetBackgroundRenderer(pTextBoxBG);

		auto pTextBoxBorder = new TexturedBorderRenderer(TextureType::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Colors::LineColor);
		SetBorderRenderer(pTextBoxBorder);
	}

	void SearchBox::OnSize()
	{
		if (_pIcon)
		{
			_pIcon->SetX(4);
			_pIcon->CenterVertically();
		}
	}
}
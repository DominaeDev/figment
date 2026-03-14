#include <pch.h>
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"
#include "gui/GUIUtility.h"

using namespace fig::gui::util;

namespace fig::gui
{
	SearchBox::SearchBox(LayoutElement* pParent, FontFace fontFace, double ptSize) : TextBox(pParent, fontFace, ptSize)
	{
		if (_pFont)
			SetSize(300, toF(MeasureFontHeight(*_pFont)) + GetMarginVertical());
		
		SetMarginLeft(30);

		_pIcon = new Image(this, AppResources::GetTexture(TextureType::ICON_SEARCH));
		_pIcon->SetForegroundColor(with_alpha(Colors::Black, 0.3f));

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

	void SearchBox::OnSize()
	{
		if (_pIcon)
		{
			_pIcon->SetX(4);
			_pIcon->CenterVertically();
		}
	}
}
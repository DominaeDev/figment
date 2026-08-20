#include <pch.h>
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	SearchBox::SearchBox(ControlPtr pParent) : TextBox(pParent, FontFace::Default, Constants::GUI::TextBoxFontSize, TextInput::Flags { TextInput::Flag::Single }), MouseEventHandler(this)
	{
		SetMarginLeft(30); // Icon
		SetMarginRight(30); // Cross

		_pIcon = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_SEARCH));
		_pIcon->SetForegroundColor(Color::Icon);

		_pCross = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_CROSS));
		_pCross->SetForegroundColor(Color::Icon);
		_pCross->SetVisible(false);

		auto pTextBoxBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Color::White);

		auto pTextBoxBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color::LineColor);
	}

	void SearchBox::OnSize()
	{
		_pIcon->SetX(4);
		_pIcon->CenterVertically();

		_pCross->SetX(GetWidth() - 4 - _pCross->GetWidth());
		_pCross->CenterVertically();
		MouseEventHandler::SetClickableRegion(_pCross->GetLocalRect());
	}

	EventResult SearchBox::OnEvent(fig::event& event)
	{
		if (auto result = MouseEventHandler::HandleMouseEvents(event); result == EventResult::Handled)
			return result;
		
		return TextBox::OnEvent(event);
	}

	void SearchBox::OnText(fig::string_view text) noexcept
	{
		_bHasText = text.length() > 0;
		_pCross->SetVisible(_bHasText);
	}

	void SearchBox::OnEnabled(bool bEnabled)
	{
		MouseEventHandler::Enable(bEnabled);
	}

	void SearchBox::OnClicked()
	{
		if (_bHasText)
			Clear();
	}
}
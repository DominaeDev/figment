#include <pch.h>
#include "gui/AddImageButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	AddImageButton::AddImageButton(ControlPtr pParent, fig::string_view label) : Control(pParent), MouseEventHandler(this)
	{
//		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
//		pBGRenderer->SetColor(0xEFECE3_rgb);
//		SetBackgroundColor(0xEFECE3_rgb);

		auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorderRenderer->SetColor(0xe0dccb_rgb);

		_pLabel = CreateControl<StaticText>(label, FontFace::Italic, Constants::GUI::DefaultFontSize, true);
		_pLabel->SetForegroundColor(0x9d9584_rgb);

		SetSize(Constants::GUI::Cards::Half::Width, Constants::GUI::Cards::Half::Height);
	}

	EventResult AddImageButton::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}

	void AddImageButton::OnSize()
	{
		_pLabel->Center();
	}

	void AddImageButton::OnMouseEnter()
	{
		PushEvent(UserEvent::PushCursor, Cursor::Pointer);
	}

	void AddImageButton::OnMouseExit()
	{
		PushEvent(UserEvent::PopCursor, Cursor::Pointer);
	}

	void AddImageButton::OnClicked()
	{
		PushEvent(UserEvent::PopCursor, Cursor::Pointer);
	}
}
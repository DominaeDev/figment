#include <pch.h>
#include "gui/AddImageButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	AddImageButton::AddImageButton(ControlPtr pParent, fig::string_view label) : Control(pParent), MouseEventHandler(this)
	{
		auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorderRenderer->SetColor(fig::color_ref(Color::Border).WithAlpha(0xC0));

		_pLabel = CreateControl<StaticText>(label, FontFace::Italic, Constants::GUI::DefaultFontSize, true);
		_pLabel->SetForegroundColor(Color::HintText);

		SetSize(Constants::GUI::CharacterEditor::PortraitWidth, Constants::GUI::CharacterEditor::PortraitHeight);
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
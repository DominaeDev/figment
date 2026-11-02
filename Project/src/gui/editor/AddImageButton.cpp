#include <pch.h>
#include "gui/editor/AddImageButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	AddImageButton::AddImageButton(ControlPtr pParent) : Control(pParent), MouseEventHandler(this)
	{
		auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorderRenderer->SetColor(fig::color_ref(Color::Border).WithAlpha(0xC0));

		_pIcon = CreateControl<Image>(Resource::ICON_PLUS);
		_pIcon->SetForegroundColor(Color::HintText);

		SetSize(Constants::GUI::CharacterEditor::PortraitWidth, Constants::GUI::CharacterEditor::PortraitHeight);
	}

	EventResult AddImageButton::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}

	void AddImageButton::OnSize()
	{
		_pIcon->Center();
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
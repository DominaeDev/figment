#include <pch.h>
#include "gui/editor/StoryRoleSlotBox.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	StoryRoleSlotBox::StoryRoleSlotBox(ControlPtr pParent) : Control(pParent), MouseEventHandler(this)
	{
		auto pBorderRenderer = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorderRenderer->SetColor(fig::color_ref(Color::Border).WithAlpha(0xC0));

		auto pBackgroundRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBackgroundRenderer->SetColor(Color::ButtonDefaultBackground);

		_pLabel = CreateControl<StaticText>("", FontFace::Default, 14.0);
		_pLabel->SetForegroundColor(Color::HintText);
		_pLabel->SetMaxWidth(102);
		_pLabel->EnableEllipsis(true);
		_pLabel->SetAlignment(TextAlignment::MiddleTop);

		_pIcon = CreateControl<Image>(Icon::ROLE_BOT);
		_pIcon->SetForegroundColor(Color::HintText);

		SetSize(114, 152);
	}

	void StoryRoleSlotBox::SetRole(fig::chat::Role role, fig::string_view label)
	{
		if (empty_or_whitespace(label))
		{
			if (is_bot(role))
				_pLabel->SetTextAndResize(std::format("Role #{}", get_bot_index(role) + 1));
			else if (is_user(role))
				_pLabel->SetTextAndResize("User");
		}
		else 
			_pLabel->SetTextAndResize(label);

		_pLabel->CenterHorizontally();

		if (is_user(role))
			_pIcon->SetTexture(Icon::ROLE_USER);
		else
			_pIcon->SetTexture(Icon::ROLE_BOT);
	}

	void StoryRoleSlotBox::OnSize()
	{
		_pIcon->Center();
		_pIcon->SetY(_pIcon->GetY() - 6);
		_pLabel->Center();
		_pLabel->SetY(_pIcon->GetBottom());
	}

	EventResult StoryRoleSlotBox::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}
}
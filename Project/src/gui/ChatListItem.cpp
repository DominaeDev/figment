#include <pch.h>
#include "gui/ChatListItem.h"
#include "gui/CustomRenderers.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"
#include "gui/Menu.h"

namespace fig::gui
{
	ChatListItem::ChatListItem(LayoutElement* pParent) : Panel(pParent)
	{
		SetMaxSize(700, -1);
		SetHeight(60);

		// Background
		SetForegroundColor(Colors::SidePanelForeground);

		auto pBGRenderer = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_10PX, 8);
		pBGRenderer->SetColor(Colors::SidePanelBackground);
		SetBackgroundRenderer(pBGRenderer);

		auto pBorder = new TexturedBorderRenderer(TextureType::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Colors::LineColor);
		SetBorderRenderer(pBorder);

		// Title
		_pTitle = new StaticText(this, "Yuki", FontFace::Bold, 14.0, false);
		_pTitle->EnableEllipsis(true);
		_pTitle->SetPosition(66, 6);

		// Message
		_pMessage = new StaticText(this, "Yuki: \"I've been meaning to talk to you for some time, but it's been really hard to get in touch with you, you know?\"", FontFace::Italic, 14.0, false);
		_pMessage->EnableEllipsis(true);
		_pMessage->SetPosition(66, 28);

		// Timestamp
		_pTimestamp = new StaticText(this, "", FontFace::Italic, 11.0, true);
		_pTimestamp->SetForegroundColor(Colors::SidePanelForeground.WithAlpha(0.5f));
		_pTimestamp->SetY(8);
		_pTimestamp->SetMaxSize(100, -1);
		_pTimestamp->SetTextAndResize("1:30 p.m.");

		// Portrait
		_pPortrait = new ImageWithMask(this, nullptr, nullptr);
		_pPortrait->SetSize(48, 48);
		_pPortrait->SetPosition(8, 6);
		_pPortrait->SetTexture(AppResources::GetTexture(TextureType::PROFILE_DEFAULT_IMAGE), AppResources::GetTexture(TextureType::CIRCLE_MASK));
	}

	void ChatListItem::OnSize()
	{
		if (_pTitle)
		{
			_pTitle->SetMaxSize(GetWidth() - _pTitle->GetX() - 112, -1);
			_pTitle->InvalidateText();
		}

		if (_pMessage)
		{
			_pMessage->SetMaxSize(GetWidth() - _pMessage->GetX() - 112, -1);
			_pMessage->InvalidateText();
		}

		if (_pTimestamp)
		{
			_pTimestamp->SetX(GetWidth() - _pTimestamp->GetWidth() - 8);
		}
	}

	EventResult ChatListItem::OnEvent(Event& event)
	{
		switch (event.type)
		{
			case SDL_EVENT_MOUSE_BUTTON_UP:
				if (event.button.button == SDL_BUTTON_RIGHT
					and is_inside(GetRect(), toI(event.button.x), toI(event.button.y)) )
				{
					ShowMenu();
					return EventResult::Handled;
				}
				break;
		}

		return EventResult::Pass;
	}

	void ChatListItem::ShowMenu() noexcept
	{
		auto& menu = MainFrame::GetInstance().CreateMenu();

		bool bLLM = Global::IsLLMInitialized();

		menu.AddItem("Resume chat\u2026", TextureType::ICON_NEW_CHAT)
			.SetEnabled(bLLM)
			.SetDelegate([this] {
//			PushEvent(UserEvent::StartChat, &_characterId);
		});

		menu.AddItem("Duplicate\u2026");
		menu.AddItem("Export\u2026");
		menu.AddSeparator();
		menu.AddItem("Star", TextureType::ICON_STAR);
		menu.AddSeparator();
		menu.AddItem("Delete\u2026");
		menu.Show();
	}
	
}
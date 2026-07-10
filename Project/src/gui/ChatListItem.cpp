#include <pch.h>
#include "gui/ChatListItem.h"
#include "gui/CustomRenderers.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"
#include "gui/Menu.h"
#include "data/ChatLog.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::gui
{
	ChatListItem::ChatListItem(ParentPtr pParent) : Panel(pParent)
	{
		SetMaxSize(760, -1);
		SetHeight(60);

		// Background
		SetForegroundColor(Colors::SidePanelForeground);

		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(TextureType::ROUNDED_BACKGROUND_10PX, 8);
		pBGRenderer->SetColor(Colors::SidePanelBackground);

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(TextureType::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Colors::LineColor);

		// Title
		_pTitle = CreateControl<StaticText>("", FontFace::Bold, 14.0, false);
		_pTitle->EnableEllipsis(true);
		_pTitle->SetPosition(66, 6);

		// Message
		_pMessage = CreateControl<StaticText>("", FontFace::Italic, 14.0, false);
		_pMessage->EnableEllipsis(true);
		_pMessage->SetPosition(66, 28);

		// Timestamp
		_pTimestamp = CreateControl<StaticText>("", FontFace::Italic, 11.0, true);
		_pTimestamp->SetForegroundColor(Colors::SidePanelForeground.WithAlpha(0.5f));
		_pTimestamp->SetY(8);
		_pTimestamp->SetMaxSize(100, -1);

		// Portrait
		_pPortrait = CreateControl<Image>(nullptr);
		_pPortrait->SetSize(48, 48);
		_pPortrait->SetPosition(8, 6);
		_pPortrait->SetVisible(false);
	}

	ChatListItem::ChatListItem(ParentPtr pParent, const ChatLog& chatLog, fig::timestamp lastUsed) : ChatListItem(pParent)
	{
		if (not empty_or_whitespace(chatLog.title))
			_pTitle->SetText(chatLog.title);
		else
			_pTitle->SetText("Untitled chat");

		if (not chatLog.messages.empty())
		{
			auto& lastMessage = chatLog.messages.back();
			auto& speakerId = lastMessage.speakerId;
			auto name = Global::GetUserManager().GetContent().GetCharacterName(lastMessage.speakerId).value_or("Unknown");

			if (lastMessage.msgType == fig::chat::MessageType::Action)
				_pMessage->SetText(std::format("{}: *{}*", name, trunc(lastMessage.content, 256uz)));
			else
				_pMessage->SetText(std::format("{}: \"{}\"", name, trunc(lastMessage.content, 256uz)));

			if (auto portrait = Global::GetUserContent().GetSmallPortraitForCharacter(speakerId, AppResources::GetTexture(TextureType::MASK_CIRCLE), GetSDLRenderer()))
			{
				_pPortrait->SetTexture((*portrait).get());
				_pPortrait->SetVisible(true);
			}
		}

		_pTimestamp->SetTextAndResize(lastUsed.get_time_string());
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

		if (IsUserEvent(event, UserEvent::MenuOpened))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = true;
				return EventResult::Continue;
			}
		}
		else if (IsUserEvent(event, UserEvent::MenuClosed))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = false;
				return EventResult::Continue;
			}
		}
		return EventResult::Pass;
	}

	void ChatListItem::OnUpdate(float fElapsed)
	{
		bool bHovered = (_bSelected or is_inside(GetRect(), GetMousePos())
			and !MainFrame::GetInstance().IsMenuShowing());

		if (_bHovered != bHovered)
		{
			_bHovered = bHovered;
			GetBackgroundRenderer()->SetColor(_bHovered ? 0xFFFFFF80_rgba : 0xEEECE480_rgba);
		}
	}

	void ChatListItem::ShowMenu() noexcept
	{
		auto& menu = MainFrame::GetInstance().CreateMenu();

		bool bLLM = Global::IsLLMInitialized();

		menu.AddItem("Resume chat", TextureType::ICON_NEW_CHAT)
			.SetEnabled(bLLM)
			.SetDelegate([this] {
//			PushEvent(UserEvent::StartChat, &_characterId);
		});
		menu.AddItem("Start another chat\u2026")
			.SetEnabled(bLLM)
			.SetDelegate([this] {
//			PushEvent(UserEvent::StartChat, &_characterId);
		});

		menu.AddSeparator();
		menu.AddItem("Star", TextureType::ICON_STAR);
		menu.AddSeparator();
		menu.AddItem("Find similar");
		menu.AddItem("Duplicate\u2026");
		menu.AddItem("Export\u2026");
		menu.AddSeparator();
		menu.AddItem("Delete\u2026");
		_menuId = menu.Show();
	}
}
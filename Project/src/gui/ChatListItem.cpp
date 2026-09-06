#include <pch.h>
#include "gui/ChatListItem.h"
#include "gui/CustomRenderers.h"
#include "gui/AppResources.h"
#include "gui/Frame.h"
#include "gui/Menu.h"
#include "data/ChatLog.h"
#include "io/ContentUserSettings.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::gui
{
	ChatListItem::ChatListItem(ControlPtr pParent) : Panel(pParent)
	{
		SetMaxSize(Constants::GUI::ChatList::Width, -1);
		SetHeight(60);

		// Background
		SetForegroundColor(Color::SidePanelForeground);
		SetBackgroundColor(0xf4f2ec_rgb);
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 8);
		pBGRenderer->SetColor(GetBackgroundColor());

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::LineColor);

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
		_pTimestamp->SetForegroundColor(Color::SidePanelForeground.WithAlpha(0.5f));
		_pTimestamp->SetY(8);
		_pTimestamp->SetMaxWidth(100);

		// Portrait
		_pPortrait = CreateControl<Image>(nullptr);
		_pPortrait->SetSize(48, 48);
		_pPortrait->SetPosition(8, 6);
		_pPortrait->SetVisible(false);
	}

	ChatListItem::ChatListItem(ControlPtr pParent, const fig::uuid& assetId, const fig::data::ChatLog& chatLog, const fig::string& timeString) : ChatListItem(pParent)
	{
		if (not empty_or_whitespace(chatLog.GetTitle()))
			_pTitle->SetText(chatLog.GetTitle());
		else
			_pTitle->SetText("Untitled chat");

		if (not chatLog.GetMessages().empty())
		{
			auto& lastMessage = chatLog.GetMessages().back();
			auto& speakerId = lastMessage.speakerId;
			_primaryCharacterId = speakerId;

			auto name = Global::GetUserContent().GetCharacterName(lastMessage.speakerId).value_or("Unknown");
			if (lastMessage.msgType == fig::chat::MessageType::Action)
				_pMessage->SetText(std::format("{}: *{}*", name, truncate(lastMessage.content, 256uz)));
			else
				_pMessage->SetText(std::format("{}: \"{}\"", name, truncate(lastMessage.content, 256uz)));

			if (auto portrait = Global::GetUserContent().GetSmallPortraitForCharacter(speakerId, AppResources::GetTexture(Resource::MASK_SMALL_PORTRAIT_48PX), GetSDLRenderer()))
			{
				_pPortrait->SetTexture((*portrait).get());
				_pPortrait->SetVisible(true);
			}

//			_createdAt = Global::GetUserContent().GetMetaData(
		}

		if (Global::GetUserContent().GetUserSettings(assetId).flags.IsSet(ContentUserSettings::Flag::Favorite))
			ShowStar(true);

		_assetId = assetId;
		_pTimestamp->SetTextAndResize(timeString);
	}

	void ChatListItem::OnSize()
	{
		if (_pTitle)
		{
			_pTitle->SetWidth(GetWidth() - _pTitle->GetX() - 112);
			_pTitle->InvalidateText();
		}

		if (_pMessage)
		{
			_pMessage->SetWidth(GetWidth() - _pMessage->GetX() - 112);
			_pMessage->InvalidateText();
		}

		if (_pTimestamp)
			_pTimestamp->SetX(GetWidth() - _pTimestamp->GetWidth() - 8);

		if (_pStar)
			_pStar->SetX(GetWidth() - _pStar->GetWidth() - 8);

	}

	EventResult ChatListItem::OnEvent(fig::event& event)
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
			and not GetOwnerFrame()->IsMenuShowing());

		if (_bHovered != bHovered)
		{
			_bHovered = bHovered;
			SetBackgroundColor(_bHovered ? 0xfdfcfa_rgb : 0xf4f2ec_rgb);
			GetBackgroundRenderer()->SetColor(_bHovered ? 0xfdfcfa_rgb : 0xf4f2ec_rgb);
		}
		_pTitle->SetBackgroundColor(GetBackgroundColor());
		_pMessage->SetBackgroundColor(GetBackgroundColor());
		_pTimestamp->SetBackgroundColor(GetBackgroundColor());
	}

	void ChatListItem::ShowMenu() noexcept
	{
		auto& menu = CreateMenu();

		bool bLLM = Global::IsLLMInitialized();

		auto userSettings = Global::GetUserContent().GetUserSettings(_assetId);

		menu.AddItem("Resume this chat", Resource::ICON_NEW_CHAT)
			.SetEnabled(bLLM)
			.SetDelegate([this] {
//			PushEvent(UserEvent::StartChat, &_characterId);
		});
		menu.AddItem("Start a new chat\u2026")
			.SetEnabled(bLLM)
			.SetDelegate([this] {
//			PushEvent(UserEvent::StartChat, &_characterId);
		});
		menu.AddItem("Filter by character")
			.SetEnabled(not _primaryCharacterId.empty())
			.SetDelegate([this]() {
				PushEvent(UserEvent::NavigateToChatList, &_primaryCharacterId);
			});

		menu.AddSeparator();
		menu.AddItem("Edit chat settings\u2026", Resource::ICON_EDIT);
		menu.AddItem("Duplicate\u2026");
		menu.AddItem("Export\u2026");
		menu.AddSeparator();

		if (!userSettings.HasFlag(ContentUserSettings::Flag::Favorite))
		{
			menu.AddItem("Star", Resource::ICON_STAR)
				.SetDelegate([this] {
				Global::GetUserContent().MarkFavorite(_assetId, true);
				ShowStar(true);
				NotifyUpdated();
			});
		}
		else
		{
			menu.AddItem("Unstar", Resource::ICON_UNSTAR)
				.SetDelegate([this] {
				Global::GetUserContent().MarkFavorite(_assetId, false);
				ShowStar(false);
				NotifyUpdated();
			});
		}

		if (!userSettings.HasFlag(ContentUserSettings::Flag::Hidden))
		{
			menu.AddItem("Archive")
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_assetId, true);
					NotifyUpdated();
				});
		}
		else
		{
			menu.AddItem("Unarchive")
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_assetId, false);
					NotifyUpdated();
				});
			menu.AddSeparator();
			menu.AddItem("Delete\u2026", Resource::ICON_DELETE)
				.SetDelegate([this] { NotifyDelete(); });
		}
		_menuId = menu.Show();
	}

	void ChatListItem::ShowStar(bool bShow)
	{
		if (!_pStar)
		{
			if (bShow)
			{
				_pStar = CreateControl<Image>(Resource::CARD_ICON_STAR_SMALL);
				_pStar->SetSize(24, 24);
				_pStar->SetY(28);
			}
		}
		else
		{
			_pStar->SetVisible(bShow);
		}
	}

	void ChatListItem::NotifyUpdated()
	{
		if (_fnDelegate)
			_fnDelegate(*this, ChatListItemEvent::Refresh);
	}

	void ChatListItem::NotifyDelete()
	{
		if (_fnDelegate)
			_fnDelegate(*this, ChatListItemEvent::Delete);
	}
}
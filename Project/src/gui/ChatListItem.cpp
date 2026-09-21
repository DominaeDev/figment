#include <pch.h>
#include "gui/ChatListItem.h"
#include "gui/CustomRenderers.h"
#include "gui/AppResources.h"
#include "gui/Frame.h"
#include "gui/Menu.h"
#include "data/ChatLog.h"
#include "io/AssetUserSettings.h"

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
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBGRenderer->SetColor(custom_color(GetBackgroundColor()));

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Colour::LineColor);

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

	ChatListItem::ChatListItem(ControlPtr pParent, const fig::uuid& chatInstanceId, const fig::data::ChatInstance& chatInstance, const fig::data::ChatLog& chatLog, const fig::string& timeString) : ChatListItem(pParent)
	{
		if (not empty_or_whitespace(chatLog.GetTitle()))
			_pTitle->SetText(chatLog.GetTitle());
		else
		{
			_pTitle->SetText("Untitled chat");
			_bHasError = true;
		}

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
		}
		else
		{
			auto& characterId = chatInstance.characterIds[static_cast<size_t>(fig::chat::Role::Bot1)];
			if (auto portrait = Global::GetUserContent().GetSmallPortraitForCharacter(characterId, AppResources::GetTexture(Resource::MASK_SMALL_PORTRAIT_48PX), GetSDLRenderer()))
			{
				_pPortrait->SetTexture((*portrait).get());
				_pPortrait->SetVisible(true);
			}
		}

		if (Global::GetUserContent().GetUserSettings(chatInstanceId).flags.IsSet(AssetUserSettings::Flag::Favorite))
			ShowStar(true);

		_chatInstanceId = chatInstanceId;
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
			GetBackgroundRenderer()->SetColor(_bHovered ? custom_color(0xfdfcfa_rgb) : custom_color(0xf4f2ec_rgb));
		}
		_pTitle->SetBackgroundColor(GetBackgroundColor());
		_pMessage->SetBackgroundColor(GetBackgroundColor());
		_pTimestamp->SetBackgroundColor(GetBackgroundColor());
	}

	void ChatListItem::ShowMenu() noexcept
	{
		auto& menu = CreateMenu();

		bool bLLM = Global::IsLLMInitialized();

		auto userSettings = Global::GetUserContent().GetUserSettings(_chatInstanceId);


		if (not userSettings.HasFlag(AssetUserSettings::Flag::Hidden))
		{
			menu.AddItem("Resume chat", Resource::ICON_NEW_CHAT)
				.SetEnabled(bLLM)
				.SetDelegate([this] {
//				PushEvent(UserEvent::StartChat, &_characterId);
			});
			menu.AddItem("Filter by character")
				.SetEnabled(not _primaryCharacterId.empty())
				.SetDelegate([this]() {
				auto flags = Global::GetUserSettings().GetChatListFilter();
				PushEvent(UserEvent::NavigateToChatList, flags.IsSet(ChatFilterFlag::Hidden) ? 1 : 0, &_primaryCharacterId);
			});

			menu.AddSeparator();

			if (!userSettings.HasFlag(AssetUserSettings::Flag::Favorite))
			{
				menu.AddItem("Star", Resource::ICON_STAR)
					.SetDelegate([this] {
					Global::GetUserContent().MarkFavorite(_chatInstanceId, true);
					ShowStar(true);
					NotifyUpdated();
				});
			}
			else
			{
				menu.AddItem("Unstar", Resource::ICON_UNSTAR)
					.SetDelegate([this] {
					Global::GetUserContent().MarkFavorite(_chatInstanceId, false);
					ShowStar(false);
					NotifyUpdated();
				});
			}
			menu.AddSeparator();

			menu.AddItem("Duplicate\u2026");
			menu.AddItem("Export\u2026");
			menu.AddSeparator();

			menu.AddItem("Archive", Resource::ICON_ARCHIVE)
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_chatInstanceId, true);
					Global::GetUserContent().InvalidateChatCount(_chatInstanceId);
					NotifyUpdated();
				});
			
			if (_bHasError)
			{
				menu.AddSeparator();
				menu.AddItem("Delete\u2026", Resource::ICON_DELETE)
					.SetDelegate([this] { NotifyDelete(); });
			}
		}
		else // Archived
		{
			menu.AddItem("Filter by character")
				.SetEnabled(not _primaryCharacterId.empty())
				.SetDelegate([this]() {
				auto flags = Global::GetUserSettings().GetChatListFilter();
				PushEvent(UserEvent::NavigateToChatList, flags.IsSet(ChatFilterFlag::Hidden) ? 1 : 0, &_primaryCharacterId);
			});

			menu.AddItem("Unarchive", Resource::ICON_UNARCHIVE)
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_chatInstanceId, false);
					Global::GetUserContent().InvalidateChatCount(_chatInstanceId);
					NotifyUpdated();
				});

			menu.AddSeparator();
			menu.AddItem("Delete forever\u2026", Resource::ICON_DELETE)
				.SetDelegate([this] { NotifyDelete(); });
			menu.AddItem("Purge archive\u2026", Resource::ICON_DELETE);

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
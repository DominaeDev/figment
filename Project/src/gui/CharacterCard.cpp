#include <pch.h>
#include "gui/CharacterCard.h"
#include "gui/AppResources.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "gui/Events.h"
#include "gui/MainFrame.h"
#include "gui/Menu.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	CharacterCard::CharacterCard(ParentPtr pParent, const fig::uuid& characterId, CardSize cardSize) : CoverCard(pParent, characterId, cardSize),
		_characterId { characterId }
	{
		if (auto try_character = Global::GetUserContent().Get<Character>(characterId); try_character.has_value())
		{
			auto& character = try_character.value();
			_characterName = character.shortName;
			SetLabel(character.fullName);
			SetIndex(character.GetSearchIndex());

			// Meta data
			if (auto try_meta = Global::GetUserContent().GetMetaData(characterId))
			{
				auto& meta = *try_meta;
				SetMetaData(meta);
			}
			else
			{
				auto now = fig::now();
				SetMetaData(ContentMetaData {
					.name = _characterName,
					.createdAt = now,
					.updatedAt = now,
					.lastUsedAt = now,
				});
			}

			// User settings
			SetUserSettings(Global::GetUserContent().GetUserSettings(characterId));
			
			// Tags
			if (character.gender.IsConventional())
			{
				fig::gui::Color color;
				if (character.gender == ConventionalGender::Male)
					color = Colors::GenderTagMale;
				else if (character.gender == ConventionalGender::Female)
					color = Colors::GenderTagFemale;
				else if (character.gender.IsConventional())
					color = Colors::GenderTagOther;
				else
					color = Colors::White;

				AddTag(character.gender.GetLabel(), color);
			}

			auto& tags = character.GetTags();
			for (size_t i = 0; i < tags.size(); ++i)
			{
				if (AddTag(tags[i]) == CoverCard::AddTagResult::Stop)
					break;
			}
		}
	}

	EventResult CharacterCard::OnEvent(Event& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event.button.button == SDL_BUTTON_RIGHT 
				and is_inside(GetRect(), toI(event.button.x), toI(event.button.y))
				and not _bHidden
				and not _bHasError)
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

	void CharacterCard::ShowMenu()
	{
		auto ChangeBorder = [this](CardBorderStyle border) {
			_userSettings.borderStyle = border;
			Global::GetUserContent().SetBorder(_characterId, border);
			SetBorder(border);
		};

		bool bLLM = Global::IsLLMInitialized();

		auto& menu = MainFrame::GetInstance().CreateMenu();

		menu.AddItem("Resume last chat")
			.SetEnabled(bLLM && _metaData.chatCount > 0);
		menu.AddItem(std::format("New chat with {}\u2026", _characterName), TextureType::ICON_NEW_CHAT)
			.SetEnabled(bLLM)
			.SetDelegate([this] { 
				PushEvent(UserEvent::StartChat, &_characterId); 
			});

		menu.AddItem("Show all chats")
			.SetEnabled(_metaData.chatCount > 0)
			.SetDelegate([this] { 
				PushEvent(UserEvent::NavigateToChatList, &_characterId); 
			});

		if constexpr (Debugging)
		{
			menu.AddItem(std::format("Debug {}\u2026", _characterName))
				.SetDelegate([this] {
				PushEvent(UserEvent::DebugCharacter, &_characterId);
			});
		}

		menu.AddSeparator();
		menu.AddItem("View / Edit\u2026");
		menu.AddItem("Duplicate\u2026");
		menu.AddItem("Export\u2026");
		auto& moveMenu = menu.AddItem("Move to folder\u2026");
			moveMenu.AddItem("New folder\u2026");
		menu.AddSeparator();
		auto& borderMenu = menu.AddItem("Set border");
			borderMenu.AddCheckItem("No border", _userSettings.borderStyle == CardBorderStyle::None)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::None); });
			borderMenu.AddSeparator();
			borderMenu.AddCheckItem("Border #1", _userSettings.borderStyle == CardBorderStyle::Style01)
				.SetIcon(TextureType::ICON_BORDER_01, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style01); });
			borderMenu.AddCheckItem("Border #2", _userSettings.borderStyle == CardBorderStyle::Style02)
				.SetIcon(TextureType::ICON_BORDER_02, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style02); });
			borderMenu.AddCheckItem("Border #3", _userSettings.borderStyle == CardBorderStyle::Style03)
				.SetIcon(TextureType::ICON_BORDER_03, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style03); });
			borderMenu.AddCheckItem("Border #4", _userSettings.borderStyle == CardBorderStyle::Style04)
				.SetIcon(TextureType::ICON_BORDER_04, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style04); });
			borderMenu.AddCheckItem("Border #5", _userSettings.borderStyle == CardBorderStyle::Style05)
				.SetIcon(TextureType::ICON_BORDER_05, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style05); });
			borderMenu.AddCheckItem("Border #6", _userSettings.borderStyle == CardBorderStyle::Style06)
				.SetIcon(TextureType::ICON_BORDER_06, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style06); });

		if (!_userSettings.HasFlag(ContentUserSettings::Flag::Favorite))
		{
			menu.AddItem("Star", TextureType::ICON_STAR)
				.SetDelegate([this] {
					Global::GetUserContent().MarkFavorite(_characterId, true);
					_userSettings.flags.Set(ContentUserSettings::Flag::Favorite);
					ShowStar(true);
					NotifyMetaUpdated();
				});
		}
		else
		{
			menu.AddItem("Unstar", TextureType::ICON_UNSTAR)
				.SetDelegate([this] {
					Global::GetUserContent().MarkFavorite(_characterId, false);
					_userSettings.flags.Unset(ContentUserSettings::Flag::Favorite);
					ShowStar(false);
					NotifyMetaUpdated();
				});
		}

		if (!_userSettings.HasFlag(ContentUserSettings::Flag::Hidden))
		{
			menu.AddItem("Hide")
				.SetDelegate([this] { 
					Global::GetUserContent().MarkHidden(_characterId, true);
					_userSettings.flags.Set(ContentUserSettings::Flag::Hidden);
					NotifyMetaUpdated();
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([this] { 
					Global::GetUserContent().MarkHidden(_characterId, false);
					_userSettings.flags.Unset(ContentUserSettings::Flag::Hidden);
					NotifyMetaUpdated();
				});
		}
		menu.AddSeparator();
		menu.AddItem("Delete\u2026", TextureType::ICON_DELETE);

		_menuId = menu.Show();
	}
}
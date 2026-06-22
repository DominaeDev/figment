#include <pch.h>
#include "gui/CharacterCard.h"
#include "gui/AppResources.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "gui/Events.h"
#include "gui/Menu.h"
#include "gui/MainFrame.h"

using namespace fig::data;

namespace fig::gui
{
	CharacterCard::CharacterCard(LayoutElement* pParent, const fig::uuid& characterId, CardSize cardSize) : CoverCard(pParent, characterId, cardSize),
		_characterId { characterId }
	{
		if (auto try_character = Global::GetUserManager().GetContent().GetCharacter(characterId); try_character.has_value())
		{
			auto& character = try_character.value();
			_characterName = character.shortName;
			SetLabel(character.fullName);
			SetIndex(character.GetSearchIndex());

			if (auto try_meta = Global::GetUserManager().GetContent().GetMetaData(characterId))
			{
				auto& meta = *try_meta;
				SetMetaData(meta);
				ShowStar(meta.flags.IsSet(CardMetaData::Flag::Favorite));
				ShowNew(meta.IsNew());
			}
			else
			{
				auto now = utc_now();
				SetMetaData(CardMetaData {
					.name = _characterName,
					.createdAt = now,
					.updatedAt = now,
					.lastUsedAt = now,
				});
			}

			// Tags
			if (character.gender.IsDefined())
			{
				fig::gui::Color color;
				if (character.gender == CharacterGender::Male)
					color = Colors::GenderTagMale;
				else if (character.gender == CharacterGender::Female)
					color = Colors::GenderTagFemale;
				else if (character.gender == CharacterGender::Other)
					color = Colors::GenderTagOther;
				else
					color = Colors::White;

				AddTag(character.gender, color);
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
			_metaData.borderStyle = border;
			Global::GetUserManager().GetContent().SetBorder(_characterId, border);
			SetBorder(border);
		};

		bool bLLM = Global::IsLLMInitialized();

		auto& menu = MainFrame::GetInstance().CreateMenu();

		menu.AddItem(std::format("Chat with {}\u2026", _characterName), TextureType::ICON_NEW_CHAT)
			.SetEnabled(bLLM)
			.SetDelegate([this] { 
				PushEvent(UserEvent::StartChat, &_characterId); 
			});
		menu.AddItem("Resume last chat")
			.SetEnabled(bLLM && _metaData.chatCount > 0);
		menu.AddItem(std::format("View chats with {}", _characterName))
			.SetEnabled(_metaData.chatCount > 0);

		if constexpr (Debugging)
		{
			menu.AddItem(std::format("Debug {}\u2026", _characterName))
				.SetDelegate([this] {
				PushEvent(UserEvent::DebugCharacter, &_characterId);
			});
		}

		menu.AddSeparator();
		menu.AddItem("View / Edit\u2026");
		menu.AddItem("Clone\u2026");
		menu.AddItem("Export\u2026");
		menu.AddItem("Move to folder\u2026");
		menu.AddSeparator();
		auto& borderMenu = menu.AddItem("Set border");
			borderMenu.AddCheckItem("No border", _metaData.borderStyle == CardBorderStyle::None)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::None); });
			borderMenu.AddSeparator();
			borderMenu.AddCheckItem("Border #1", _metaData.borderStyle == CardBorderStyle::Style01)
				.SetIcon(TextureType::ICON_BORDER_01, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style01); });
			borderMenu.AddCheckItem("Border #2", _metaData.borderStyle == CardBorderStyle::Style02)
				.SetIcon(TextureType::ICON_BORDER_02, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style02); });
			borderMenu.AddCheckItem("Border #3", _metaData.borderStyle == CardBorderStyle::Style03)
				.SetIcon(TextureType::ICON_BORDER_03, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style03); });
			borderMenu.AddCheckItem("Border #4", _metaData.borderStyle == CardBorderStyle::Style04)
				.SetIcon(TextureType::ICON_BORDER_04, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style04); });
			borderMenu.AddCheckItem("Border #5", _metaData.borderStyle == CardBorderStyle::Style05)
				.SetIcon(TextureType::ICON_BORDER_05, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style05); });
			borderMenu.AddCheckItem("Border #6", _metaData.borderStyle == CardBorderStyle::Style06)
				.SetIcon(TextureType::ICON_BORDER_06, false)
				.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style06); });

		if (!_metaData.flags.IsSet(CardMetaData::Flag::Favorite))
		{
			menu.AddItem("Star", TextureType::ICON_STAR)
				.SetDelegate([this] {
					Global::GetUserManager().GetContent().MarkFavorite(_characterId, true);
					_metaData.flags.Set(CardMetaData::Flag::Favorite);
					ShowStar(true);
					NotifyMetaUpdated();
				});
		}
		else
		{
			menu.AddItem("Unstar", TextureType::ICON_UNSTAR)
				.SetDelegate([this] {
					Global::GetUserManager().GetContent().MarkFavorite(_characterId, false);
					_metaData.flags.Unset(CardMetaData::Flag::Favorite);
					ShowStar(false);
					NotifyMetaUpdated();
				});
		}
		if (!_metaData.flags.IsSet(CardMetaData::Flag::Hidden))
		{
			menu.AddItem("Hide")
				.SetDelegate([this] { 
					Global::GetUserManager().GetContent().MarkHidden(_characterId, true);
					_metaData.flags.Set(CardMetaData::Flag::Hidden);
					NotifyMetaUpdated();
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([this] { 
					Global::GetUserManager().GetContent().MarkHidden(_characterId, false);
					_metaData.flags.Unset(CardMetaData::Flag::Hidden);
					NotifyMetaUpdated();
				});
		}
		menu.AddSeparator();
		menu.AddItem("Delete\u2026");

		_menuId = menu.Show();
	}
}
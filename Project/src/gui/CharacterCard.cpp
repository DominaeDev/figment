#include <pch.h>
#include "gui/CharacterCard.h"
#include "gui/AppResources.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "util/StringUtility.h"
#include "gui/Events.h"
#include "gui/Menu.h"
#include "gui/MainFrame.h"

using namespace fig::io;
using namespace fig::util;

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
			SetIndex(character.searchIndex);

			if (auto meta = Global::GetUserManager().GetContent().GetMetaData(characterId))
				SetMetaData(meta.value().get());
			else
				SetMetaData(CardMetaData {
					.name = _characterName,
					.createdAt = character.createdAt,
					.updatedAt = character.updatedAt,
					.lastUsedAt = character.updatedAt,
				});

			// Tags
			switch (character.gender)
			{
			case CharacterGender::Male:
				AddTag("Male");
				break;
			case CharacterGender::Female:
				AddTag("Female");
				break;
			case CharacterGender::Custom:
				AddTag(character.properties[Constants::CharacterProperties::Gender].value);
				break;
			}

			for (size_t i = 0; i < character.tags.size() && i < 16; ++i)
			{
				if (AddTag(character.tags[i]) == CoverCard::AddTagResult::Stop)
					break;
			}
		}
	}

	bool CharacterCard::OnEvent(Event& event)
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
				return true;
			}
			break;
		}

		if (event.type == USER_EVENT(EventType::MenuOpened))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = true;
				return true;
			}
		}
		else if (event.type == USER_EVENT(EventType::MenuClosed))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = false;
				return true;
			}
		}

		return false;
	}

	void CharacterCard::ShowMenu()
	{
		auto ChangeBorder = [this](CardBorderStyle border) {
			_metaData.borderStyle = border;
			Global::GetUserManager().GetContent().SetBorder(_characterId, border);
			SetBorder(border);
		};

		auto& menu = MainFrame::GetInstance().CreateMenu();
		menu.AddItem(std::format("Chat with {}\u2026", _characterName), TextureType::ICON_NEW_CHAT);
		menu.AddItem("Resume last chat")
			.SetEnabled(false);
		menu.AddItem(std::format("View chats with {}", _characterName))
			.SetEnabled(false);
		menu.AddSeparator();
		menu.AddItem("View / Edit\u2026");
		menu.AddItem("Clone\u2026");
		menu.AddItem("Move to folder\u2026");
		menu.AddItem("Export\u2026");
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
					DidUpdate();
				});
		}
		else
		{
			menu.AddItem("Unstar", TextureType::ICON_UNSTAR)
				.SetDelegate([this] {
					Global::GetUserManager().GetContent().MarkFavorite(_characterId, false);
					_metaData.flags.Unset(CardMetaData::Flag::Favorite);
					DidUpdate();
				});
		}
		if (!_metaData.flags.IsSet(CardMetaData::Flag::Hidden))
		{
			menu.AddItem("Hide", TextureType::ICON_HIDE)
				.SetDelegate([this] { 
					Global::GetUserManager().GetContent().MarkHidden(_characterId, true);
					_metaData.flags.Set(CardMetaData::Flag::Hidden);
					DidUpdate();
				});
		}
		else
		{
			menu.AddItem("Unhide", TextureType::ICON_UNHIDE)
				.SetDelegate([this] { 
					Global::GetUserManager().GetContent().MarkHidden(_characterId, false);
					_metaData.flags.Unset(CardMetaData::Flag::Hidden);
					DidUpdate();
				});
		}
		menu.AddSeparator();
		menu.AddItem("Delete\u2026");

		_menuId = menu.Show();
	}
}
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
			borderMenu.AddCheckItem("No border", true);
			borderMenu.AddSeparator();
			borderMenu.AddCheckItem("Border #1");
			borderMenu.AddCheckItem("Border #2");
			borderMenu.AddCheckItem("Border #3");
			borderMenu.AddCheckItem("Border #4");

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
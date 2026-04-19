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
		auto pMenu = new Menu(&MainFrame::GetInstance());
		pMenu->AddItem(std::format("Chat with {}\u2026", _characterName), TextureType::ICON_NEW_CHAT);
		pMenu->AddItem("Resume last chat")
			.SetEnabled(false);
		pMenu->AddItem(std::format("View chats with {}", _characterName))
			.SetEnabled(false);
		pMenu->AddSeparator();
		pMenu->AddItem("View / Edit\u2026");
		pMenu->AddItem("Clone\u2026");
		pMenu->AddItem("Move to folder\u2026");
		pMenu->AddItem("Export\u2026");
		pMenu->AddSeparator();
		auto& borderMenu = pMenu->AddItem("Set border");
			borderMenu.AddCheckItem("No border", true);
			borderMenu.AddSeparator();
			borderMenu.AddCheckItem("Border #1");
			borderMenu.AddCheckItem("Border #2");
			borderMenu.AddCheckItem("Border #3");
			borderMenu.AddCheckItem("Border #4");
		pMenu->AddItem("Star");
		pMenu->AddItem("Hide");
		pMenu->AddSeparator();
		pMenu->AddItem("Delete\u2026")
			.SetDelegate([]() { MainFrame::GetInstance().Close(); });

		_menuId = pMenu->Show();
	}
}
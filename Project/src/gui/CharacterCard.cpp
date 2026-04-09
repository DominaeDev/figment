#include <pch.h>
#include "gui/CharacterCard.h"
#include "gui/AppResources.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "util/StringUtility.h"
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
		if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
		{
			if (event.button.button == SDL_BUTTON_RIGHT and is_inside(GetRect(), toI(event.button.x), toI(event.button.y)))
			{
				ShowMenu();
				return true;
			}
		}
		return false;
	}

	void CharacterCard::ShowMenu()
	{
		MainFrame::GetInstance().DestroyOverlays();

		auto pMenu = new Menu(&MainFrame::GetInstance());
		pMenu->AddItem(std::format("Chat with {}", _characterName));
		pMenu->AddItem("Resume chat\u2026")
			.SetEnabled(false);
		pMenu->AddSeparator();
		pMenu->AddItem("Edit character\u2026");
		pMenu->AddItem("Set border\u2026");
		pMenu->AddItem("Duplicate\u2026");
		pMenu->AddItem("Export\u2026");
		pMenu->AddItem("Move to folder\u2026");
		pMenu->AddSeparator();
		pMenu->AddItem("Delete character\u2026")
			.SetDelegate([]() { MainFrame::GetInstance().Close(); });

		pMenu->Show();
	}
}
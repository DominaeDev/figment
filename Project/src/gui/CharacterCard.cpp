#include <pch.h>
#include "gui/CharacterCard.h"
#include "gui/AppResources.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "util/StringUtility.h"

using namespace fig::io;
using namespace fig::util;

namespace fig::gui
{
	CharacterCard::CharacterCard(LayoutElement* pParent, const fig::uuid& characterId) : CoverCard(pParent, characterId),
		_characterId { characterId }
	{
		_searchWords = std::make_unique<Dictionary>();

		if (auto try_character = Global::GetUserManager().GetContent().GetCharacter(characterId); try_character.has_value())
		{
			auto& character = try_character.value();
			SetLabel(character.fullName);
//			SetSublabel(character.value().subheader);

			CreateChatCounter(0);

			for (auto& tag : character.tags)
				AddTag(tag);

			AddSearchText(character.shortName);
			AddSearchText(character.fullName);
			AddSearchText(character.description);
			AddSearchText(character.tags);

			switch (character.gender)
			{
			case data::CharacterGender::Male:
				AddSearchText("male");
				AddSearchText("m");
				break;
			case data::CharacterGender::Female:
				AddSearchText("female");
				AddSearchText("f");
				break;
			}

			Color tagColor = { 0x31, 0x90, 0xc8 };
			for (auto& tag : character.tags)
			{
				if (not AddTag(tag, tagColor))
					break;
			}
		}

//		AddTag("#Tag", Color { 0x31, 0x90, 0xc8 });
//		AddTag("#Another tag", Color { 0xc8, 0x31, 0xad });
//		AddTag("#Yet another", Color { 0x45, 0xc8, 0x45 });
	}


}
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
	CharacterCard::CharacterCard(LayoutElement* pParent, const fig::uuid& characterId, CardSize cardSize) : CoverCard(pParent, characterId, cardSize),
		_characterId { characterId }
	{
		if (auto try_character = Global::GetUserManager().GetContent().GetCharacter(characterId); try_character.has_value())
		{
			auto& character = try_character.value();
			SetLabel(character.fullName);

			CreateChatCounter(0);

			// Tags
			switch (character.gender)
			{
			case data::CharacterGender::Male:
				AddTag("Male");
				break;
			case data::CharacterGender::Female:
				AddTag("Female");
				break;
			case data::CharacterGender::Custom:
				AddTag(character.properties[Constants::CharacterProperties::Gender].value);
				break;
			}

			for (size_t i = 0; i < character.tags.size() && i < 16; ++i)
			{
				if (AddTag(character.tags[i]) == CoverCard::AddTagResult::Stop)
					break;
			}

			if constexpr (Disabled)
			{
				switch (character.gender)
				{
				case data::CharacterGender::Male:
					AddSearchTerms("male");
					break;
				case data::CharacterGender::Female:
					AddSearchTerms("female");
					break;
				case data::CharacterGender::Custom:
					AddSearchTerms(character.properties[Constants::CharacterProperties::Gender].value);
					break;
				}

				AddSearchTerms(character.shortName);
				AddSearchTerms(character.fullName);
				AddSearchTerms(character.description);
				AddSearchTerms(character.tags);
			}
			else
			{
				SetIndex(character.searchIndex);
			}
		}
	}


}
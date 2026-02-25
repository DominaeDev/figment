#include <pch.h>
#include "gui/CharacterCard.h"
#include "gui/TextureStore.h"
#include "model/AppState.h"
#include "model/UserManager.h"

using namespace fig::fs;

namespace fig::gui
{
	CharacterCard::CharacterCard(Control* pParent, const fig::uuid& characterId) : CoverCard(pParent, characterId),
		_characterId { characterId }
	{
		if (auto character = ApplicationState::GetUserManager().GetContent().GetCharacter(characterId))
		{
			SetLabel(character.value().fullName);
//			SetSublabel(character.value().subheader);
		}

		CreateChatCounter(0);
		AddTag("#Tag", Color { 0x31, 0x90, 0xc8 });
		AddTag("#Another tag", Color { 0xc8, 0x31, 0xad });
		AddTag("#Yet another", Color { 0x45, 0xc8, 0x45 });
	}
}
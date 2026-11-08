#include <pch.h>
#include "gui/CharacterCard.h"
#include "gui/AppResources.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "gui/Events.h"
#include "gui/Menu.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	CharacterCard::CharacterCard(ControlPtr pParent, const fig::uuid& characterId, CardSize cardSize) : CoverCard(pParent, characterId, cardSize)
	{
		SetDefaultImage(AppResources::GetTexture(Resource::CARD_COVER_CHARACTER));
		RefreshFull();
	}

	void CharacterCard::RefreshMeta()
	{
		auto& userContent = Global::GetUserContent();

		// Meta data
		if (auto try_meta = userContent.GetMetaData(_assetId))
		{
			auto& meta = *try_meta;
			SetMetaData(meta);
		}
		else // Error
		{
			auto now = fig::now();
			SetMetaData(ContentMetaData {
				.name = _name,
				.createdAt = now,
				.updatedAt = now,
				.lastUsedAt = now,
			});
		}

		// Chat count
		SetChatCount(userContent.GetChatCount(_assetId));

		// User settings
		SetUserSettings(Global::GetUserContent().GetUserSettings(_assetId));
	}

	void CharacterCard::RefreshFull()
	{
		if (auto try_character = Global::GetUserContent().Get<Character>(_assetId); try_character.has_value())
		{
			auto& character = try_character.value();
			_name = character.name.GetSpokenName();
			_updatedAt = Global::GetUserContent().GetUpdatedAt(_assetId).value_or({});
			SetLabel(character.name.GetFullName());
			SetIndex(character.GetSearchIndex());

			// Tags
			ClearTags();
			if (character.gender.IsConventional())
			{
				fig::color_ref color;
				if (character.gender == ConventionalGender::Male)
					color = Color::GenderTagMale;
				else if (character.gender == ConventionalGender::Female)
					color = Color::GenderTagFemale;
				else if (character.gender.IsConventional())
					color = Color::GenderTagOther;
				else
					color = Color::White;

				AddTag(character.gender.GetLabel(), color);
			}

			auto& tags = character.GetTags();
			for (size_t i = 0; i < tags.size(); ++i)
			{
				if (AddTag(tags[i]) == CoverCard::AddTagResult::Stop)
					break;
			}
		}

		RefreshMeta();
	}

}
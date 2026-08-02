#include <pch.h>
#include <json.hpp>
#include "io/CardImporter.h"
#include "io/FileUtility.h"
#include "data/TavernCardV2.h"

using json = nlohmann::json;

using namespace fig::data;

namespace fig::io
{
	static fig::string& replace_placeholders(fig::string& text)
	{
		replace_all_inplace(text, "{{char}}", "{char}");
		replace_all_inplace(text, "{{user}}", "{user}");
		return text;
	}

	std::expected<Character, FileError> CardImporter::Import(fig::path filename) noexcept
	{
		if (auto try_import = fig::io::ReadPNGMeta(filename, "chara", true))
		{
			auto& json = try_import.value();
			replace_placeholders(json);

			TavernCardV2 card;
			card.Parse(json);

			Character character;
			character.name = CharacterName { card.data.name };
			character.description = card.data.creator_notes;
			character.AddAttribute(toStr(Constants::CharacterAttributes::Persona), "Persona", card.data.persona, CharacterAttribute::Format::Text, CharacterAttribute::Visibility::Private);
			character.AddAttribute(toStr(Constants::CharacterAttributes::Personality), "Personality", card.data.personality, CharacterAttribute::Format::Text, CharacterAttribute::Visibility::Public);
			character.AppendTags(card.data.tags);

			// Check for gender tags
			for (auto& tag : card.data.tags)
			{
				if (equals(tag, "male", true) || equals(tag, "man", true) || equals(tag, "boy", true))
				{
					character.gender = Gender::Male;
					break;
				}
				else if (equals(tag, "female", true) || equals(tag, "woman", true) || equals(tag, "girl", true))
				{
					character.gender = Gender::Female;
					break;
				}
				else if (auto gender = Gender(tag); gender.IsConventional())
				{
					character.gender = gender;
					break;
				}
			}

			if (character.gender.IsConventional())
				character.AddSearchTerm(character.gender.GetLabel());

			character.AddSearchTerm(character.name.GetFullName());
			character.AddSearchTerm(card.data.persona);
			character.AddSearchTerm(card.data.personality);

			return character;
		}
		else
		{
			return std::unexpected(try_import.error());
		}
	}
}
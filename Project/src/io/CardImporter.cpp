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

	std::expected<CharacterData, FileError> CardImporter::Import(fig::path filename) noexcept
	{
		if (auto try_import = fig::io::ReadPNGMeta(filename, "chara", true))
		{
			auto& json = try_import.value();
			replace_placeholders(json);

			TavernCardV2 card;
			card.Parse(json);

			CharacterData character;
			character.shortName = card.data.name;
			character.fullName = card.data.name;
			character.AddAttribute(toStr(Constants::CharacterAttributes::Persona), "Persona", card.data.persona, CharacterAttribute::Format::Text, CharacterAttribute::Visibility::Private);
			character.AddAttribute(toStr(Constants::CharacterAttributes::Personality), "Personality", card.data.personality, CharacterAttribute::Format::Text, CharacterAttribute::Visibility::Public);
			character.AppendTags(card.data.tags);

			// Check for gender tags
			for (auto& tag : card.data.tags)
			{
				if (equals(tag, "male", true) || equals(tag, "man", true) || equals(tag, "boy", true))
				{
					character.gender = CharacterGender::Male;
					break;
				}
				else if (equals(tag, "female", true) || equals(tag, "woman", true) || equals(tag, "girl", true))
				{
					character.gender = CharacterGender::Female;
					break;
				}
				else if (auto itFind = std::ranges::find_if(CharacterGender::AlternativeLabels, [&tag](auto& s) { return equals(tag, s, true); }); itFind != CharacterGender::AlternativeLabels.cend())
				{
					character.gender = toStr(*itFind);
					break;
				}
			}

			if (character.gender.IsDefined())
				character.AddSearchTerm(character.gender);

			character.AddSearchTerm(character.shortName);
			character.AddSearchTerm(character.fullName);
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
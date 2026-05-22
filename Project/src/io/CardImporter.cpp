#include <pch.h>
#include <json.hpp>
#include "io/CardImporter.h"
#include "io/FileUtility.h"
#include "data/TavernCardV2.h"

using json = nlohmann::json;

using namespace fig::data;

namespace fig::io
{
	std::expected<CharacterData, FileError> CardImporter::Import(fig::path filename) noexcept
	{
		auto try_import = fig::io::ReadPNGMeta(filename, "chara", true);
		if (not try_import.has_value())
			return std::unexpected(try_import.error());

		TavernCardV2 card;
		card.Parse(try_import.value());
		
		CharacterData character;
		character.shortName = card.data.name;
		character.fullName = card.data.name;
		character.description = card.data.description;
		character.tags = card.data.tags;

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
			else
			{
				for (auto& custom_gender : CharacterGender::AlternativeLabels)
				{
					if (equals(tag, custom_gender, true))
					{
						character.gender = toStr(custom_gender);
						break;
					}
				}
			}
		}

		if (character.gender.IsDefined())
			character.searchIndex.AddTerm(lcase(character.gender));

		character.searchIndex.AddTerm(character.shortName);
		character.searchIndex.AddTerm(character.fullName);
		character.searchIndex.AddTerm(character.description);
		character.searchIndex.AddTerms(character.tags);

		return character;
	}
}
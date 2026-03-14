#include <pch.h>
#include <json.hpp>
#include "fs/CardImporter.h"
#include "fs/FileUtility.h"
#include "fs/TavernCardV2.h"

using namespace fig::io::data;
using json = nlohmann::json;

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
		character.fullName = card.data.name;
		character.description = card.data.description;
		character.tags = card.data.tags;

		// Check for gender tags
		for (auto& tag : card.data.tags)
		{
			if (util::equals(tag, "male", true) || util::equals(tag, "man", true) || util::equals(tag, "boy", true))
			{
				character.gender = CharacterGender::Male;
				break;
			}
			else if (util::equals(tag, "female", true) || util::equals(tag, "woman", true) || util::equals(tag, "girl", true))
			{
				character.gender = CharacterGender::Female;
				break;
			}

			static const std::array<fig::string, 4> custom_genders = { "Futanari", "Shemale", "Trans", "Asexual" };
			for (auto& custom_gender : custom_genders)
			{
				if (util::equals(tag, custom_gender, true))
				{
					character.gender = CharacterGender::Custom;
					character.properties[Constants::CharacterProperties::Gender] = CharacterProperty {
						.label = "Gender",
						.value = custom_gender,
					};
					break;
				}
			}
		}

		return character;
	}
}
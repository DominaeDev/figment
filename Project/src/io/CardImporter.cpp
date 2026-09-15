#include <pch.h>
#include <json.hpp>
#include "io/CardImporter.h"
#include "io/FileUtility.h"
#include "data/TavernCardV2.h"
#include "util/StripHtml.h"

using namespace fig::data;

namespace fig::io
{
	static fig::string& replace_placeholders(fig::string& text)
	{
		replace_all_inplace(text, "{{char}}", "{char}");
		replace_all_inplace(text, "{{user}}", "{user}");
		return text;
	}

	static void filter_math_alphanumerics(fig::string& text) noexcept
	{
		// I came across a card on chub that used the Mathematical Alphanumeric Symbols Block (Unicode)
		// in the creator's notes, because it has a fancier font, I guess? 
		// Figment does not support it, and it causes severe performance degradation, 
		// so it's being filtered out here.

		size_t read = 0uz;
		size_t write = 0uz;
		while (read < text.size())
		{
			if (read + 3 < text.size()
				and static_cast<unsigned char>(text[read + 0]) == 0xF0
				and static_cast<unsigned char>(text[read + 1]) == 0x9D
				and static_cast<unsigned char>(text[read + 2]) >= 0x90
				and static_cast<unsigned char>(text[read + 2]) <= 0x9F)
			{
				// Skip
				read += 4;
				continue;
			}

			text[write++] = text[read++];
		}
		text.resize(write);
	}

	static fig::string& filter_text(fig::string& text)
	{
		if (contains_html(text))
			text = strip_html(text); // Remove all HTML
		filter_math_alphanumerics(text);
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
			character.creator = card.data.creator;
			character.about = filter_text(card.data.creator_notes);
			character.version = card.data.character_version;
			if (character.version == "main")
				character.version = "1.0";
			
			if (not card.data.persona.empty())
				character.SetAttribute(toStr(Constants::CharacterAttributes::Persona), "Persona", card.data.persona, CharacterAttribute::ValueType::LongText, CharacterAttribute::Visibility::Private, CharacterAttribute::HintFlags { CharacterAttribute::HintFlag::Important } );
			if (not card.data.personality.empty())
				character.SetAttribute(toStr(Constants::CharacterAttributes::Personality), "Personality", card.data.personality, CharacterAttribute::ValueType::ShortText, CharacterAttribute::Visibility::Public);
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
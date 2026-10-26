#include <pch.h>
#include <json.hpp>
#include "io/CardImporter.h"
#include "io/FileUtility.h"
#include "io/ContentManager.h"
#include "gui/Events.h"
#include "user/UserManager.h"
#include "data/TavernCardV2.h"
#include "util/StripHtml.h"

using namespace fig::chat;
using namespace fig::data;
using namespace fig::gui;

namespace fig::io
{
	static fig::string& replace_placeholders(fig::string& text)
	{
		replace_all_inplace(text, "{{char}}", "{character}");
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

	std::expected<std::pair<Character, Scenario>, FileError> CardImporter::Import(fig::path filename) noexcept
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
				character.SetAttribute(Constants::CharacterAttributes::Persona, "Persona", card.data.persona, CharacterAttribute::ValueType::LongText, CharacterAttribute::Visibility::Private, CharacterAttribute::HintFlags { CharacterAttribute::HintFlag::Important } );
			if (not card.data.personality.empty())
				character.SetAttribute(Constants::CharacterAttributes::Personality, "Personality", card.data.personality, CharacterAttribute::ValueType::ShortText, CharacterAttribute::Visibility::Public);

			character.AppendTags(card.data.tags);

			// Check for gender tags
			auto male_tags = { "male", "man", "boy" };
			auto female_tags = { "female", "woman", "girl" };
			for (auto& tag : card.data.tags)
			{
				if (std::ranges::contains(male_tags, lcase(tag)))
				{
					character.gender = Gender::Male;
					break;
				}
				else if (std::ranges::contains(female_tags, lcase(tag)))
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

			character.RebuildSearchIndex();

			// Scenario
			Scenario scenario {};
			
			if (not card.data.scenario.empty() or not card.data.greeting.empty())
			{
				scenario.name = scenario.title = std::format("{}'s story", character.name.GetSpokenName());
				scenario.creator = character.creator;

				scenario.roles.push_back(fig::data::Scenario::RoleSlot {
					.id = "character",
					.label = character.name.GetSpokenName(),
					.flags = { fig::data::Scenario::RoleSlot::Flag::Required },
				});
				scenario.roles.push_back(fig::data::Scenario::RoleSlot {
					.id = "user",
					.label = "User",
					.flags = { fig::data::Scenario::RoleSlot::Flag::Required, fig::data::Scenario::RoleSlot::Flag::User },
				});

				Scenario::Scene scene {
					.id = _CreateUUID(),
					.name = "Greeting",
					.content = card.data.scenario,
					.ttl = 15,
					.bEntryPoint = true,
				};

				if (not card.data.greeting.empty())
				{
					scene.events.push_back(Scenario::Event {
						.type = Scenario::Event::Type::ChatMessage,
						.role = Role::Bot1,
						.msgType = MessageType::Dialogue,
						.content = card.data.greeting,
					});
				}

				scenario.scenes.push_back(scene);
			}

			return std::pair { character, scenario };
		}
		else
		{
			return std::unexpected(try_import.error());
		}
	}

	void SDLCALL OnFileDialogResult(void* userdata, const char* const* fileList, int filter)
	{
		if (not fileList)
			return;

		if (not Global::IsSignedIn())
			return;

		std::vector<fig::string> paths;
		for (const char* const* path = fileList; *path; ++path)
			paths.push_back(std::filesystem::u8path(*path).u8string());

		PushEvent(UserEvent::ImportCharacter, encode_csv(paths));
	}

	void CardImporter::ShowImportDialog(fig::window_ptr pWindow) noexcept
	{
		static constexpr SDL_DialogFileFilter filters[] =
		{
			{ "Character card v2 (.png)", "png" },
		};

		SDL_ShowOpenFileDialog(OnFileDialogResult, NULL, pWindow, filters, SDL_arraysize(filters), nullptr, true);
	}
}
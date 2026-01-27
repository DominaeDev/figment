#include <pch.h>
#include "model/ChatSession.h"
#include "model/AppState.h"

#include "util/Common.h"
#include "util/StringUtility.h"
#include "util/FileUtility.h"
#include "llm/LLMUtility.h"
#include "gui/GUIUtility.h"
#include "gui/Window.h"
#include "gui/CharacterImageStore.h"

#include <exception>
#include <cassert>
#include <format>

using namespace fig::gui;
using namespace fig::gui_util;
using namespace fig::string_util;
using namespace fig::fs;

namespace fig::data
{
	bool ChatSession::Initialize(ChatOptions options)
	{
		_options = options;

		// System prompt
		_system_prompt_solo = ReadTextFile("./resources/prompting/prompt_system_solo.txt").value_or("");
		_system_prompt_group = ReadTextFile("./resources/prompting/prompt_system_group.txt").value_or("");

		// System prompt (character)
		_system_prompt_character = ReadTextFile("./resources/prompting/prompt_system_character.txt").value_or("");

		// System prompt (user)
		_system_prompt_user = ReadTextFile("./resources/prompting/prompt_system_user.txt").value_or("");

		// Formatting spec
		_formatting_solo = ReadTextFile("./resources/prompting/prompt_formatting_solo.txt").value_or("");
		_formatting_group = ReadTextFile("./resources/prompting/prompt_formatting_group.txt").value_or("");

		// State tracking
		_formatting_state = ReadTextFile("./resources/prompting/prompt_formatting_state.txt").value_or("");

		// Director prompt
		_formatting_director = ReadTextFile("./resources/prompting/prompt_formatting_director.txt").value_or("");

		// (Optional) Uncensored instructions
		_system_prompt_uncensored = ReadTextFile("./resources/prompting/prompt_system_uncensored.txt").value_or("");

		return !_system_prompt_solo.empty()
			&& !_system_prompt_character.empty()
			&& !_system_prompt_user.empty();
	}

	bool ChatSession::LoadCharacter(Role role, fig::string filename)
	{
		auto pRenderer = ApplicationState::GetMainWindow().GetSDLRenderer().get();

		Character character;
		if (character.LoadFromXml(filename))
		{
			if (role == Role::User)
				character.characterId = "USR";

			if (!empty_or_whitespace(character.portraitFilename))
				CharacterImageStore::LoadCharacterPortrait(pRenderer, character.characterId, "./characters/" + character.portraitFilename);
			_characters[role] = std::move(character);
			return true;
		}
		return false;
	}

	std::optional<Character> ChatSession::GetCharacter(Role role) const
	{
		auto itFind = _characters.find(role);
		if (itFind != _characters.end())
			return itFind->second;
		return std::nullopt;
	}

	std::optional<Character> ChatSession::GetCharacterById(fig::string identifier) const
	{
		if (identifier.empty() || _characters.empty())
			return std::nullopt;

		auto itFind = std::find_if(_characters.begin(), _characters.end(), [identifier](const auto& kvp) {
			return equals(kvp.second.characterId, identifier, true);
		});
		if (itFind != _characters.end())
			return itFind->second;
		return std::nullopt;
	}

	std::optional<Character> ChatSession::GetCharacterByName(fig::string name) const
	{
		if (name.empty() || _characters.empty())
			return std::nullopt;

		auto itFind = std::find_if(_characters.begin(), _characters.end(), [name](const auto& kvp) {
			return equals(kvp.second.shortName, name, true);
		});
		if (itFind != _characters.end())
			return itFind->second;
		return std::nullopt;
	}

	Role ChatSession::GetRoleOf(fig::string characterId) const
	{
		if (characterId.empty() || _characters.empty())
			return Role::Undefined;

		auto itFind = std::find_if(_characters.begin(), _characters.end(), [characterId](const auto& kvp) {
			return equals(kvp.second.characterId, characterId, true) || equals(kvp.second.shortName, characterId, true);
		});
		if (itFind != _characters.end())
			return itFind->first;
		return Role::Undefined;
	}

	fig::string ChatSession::GetIdentifierOf(Role role) const
	{
		if (role == Role::User)
			return "USR";
		auto optCharacter = GetCharacter(role);
		if (optCharacter.has_value())
			return ucase(optCharacter.value().characterId);
		return "_UNK";
	}

	fig::string ChatSession::GetNameOf(Role role) const
	{
		if (role == Role::System)
			return "system";
		if (role == Role::Narrator)
			return "Narrator";
		if (role == Role::Director)
			return "Director";
		auto optCharacter = GetCharacter(role);
		if (optCharacter.has_value())
			return optCharacter.value().shortName;
		return "Unknown";
	}

	std::pair<fig::gui::Color, fig::gui::Color> ChatSession::GetColorsOf(Role role) const
	{
		if (auto character = GetCharacter(role))
		{
			if (is_defined(character.value().bgColor) && is_defined(character.value().borderColor))
				return std::make_pair(character.value().bgColor, character.value().borderColor);
		}

		if (is_bot(role))
			return std::make_pair(Colors::DefaultBotMessageBackgrounds[get_bot_index(role) % 8], Colors::DefaultBotMessageBorders[get_bot_index(role) % 8]);
		else if (role == Role::User)
			return std::make_pair(Colors::DefaultUserMessageBackground, Colors::DefaultUserMessageBorder);
		else if (role == Role::System)
			return std::make_pair(Colors::MessageBackgroundNavy, Colors::MessageBorderNavy);
		else
			return std::make_pair(Colors::MessageBackgroundDefault, Colors::MessageBorderDefault);
	}

	fig::string ChatSession::GetBriefOf(Role role) const
	{
		auto optCharacter = GetCharacter(role);
		if (!optCharacter.has_value())
			return "";

		fig::string brief = trim(optCharacter.value().brief);
		replace_all_inplace(brief, "{{user}}", GetNameOf(Role::User));
		replace_all_inplace(brief, "{{char}}", optCharacter.value().shortName);
		return brief;
	}

	fig::string ChatSession::GetPersonaOf(Role role) const
	{
		auto optCharacter = GetCharacter(role);
		if (!optCharacter.has_value())
			return "";

		fig::string description = trim(optCharacter.value().description);
		if (description.empty())
			return "";

		// Format
		if (role == Role::User)
		{
			fig::string prompt = _system_prompt_user;
			replace_all_inplace(prompt, "##PERSONA##", description);
			return ApplyNames(prompt);
		}
		else
		{
			fig::string prompt = _system_prompt_character;
			replace_all_inplace(prompt, "##PERSONA##", description);
			replace_all_inplace(prompt, "{{user}}", GetNameOf(Role::User));
			replace_all_inplace(prompt, "{{char}}", optCharacter.value().shortName);
			return prompt;
		}
	}

	fig::string ChatSession::ApplyNames(fig::string text) const
	{
		replace_all_inplace(text, "{{user}}", GetNameOf(Role::User));
		replace_all_inplace(text, "{{char}}", GetNameOf(Role::Bot1));
		replace_all_inplace(text, "{{user:name}}", GetNameOf(Role::User));
		replace_all_inplace(text, "{{char:name}}", GetNameOf(Role::Bot1));

		replace_all_inplace(text, "{{char1:name}}", GetNameOf(Role::Bot1));
		replace_all_inplace(text, "{{char2:name}}", GetNameOf(Role::Bot2));
		replace_all_inplace(text, "{{char3:name}}", GetNameOf(Role::Bot3));
		replace_all_inplace(text, "{{char4:name}}", GetNameOf(Role::Bot4));
		replace_all_inplace(text, "{{char5:name}}", GetNameOf(Role::Bot5));
		replace_all_inplace(text, "{{char6:name}}", GetNameOf(Role::Bot6));
		replace_all_inplace(text, "{{char7:name}}", GetNameOf(Role::Bot7));
		replace_all_inplace(text, "{{char8:name}}", GetNameOf(Role::Bot8));

		replace_all_inplace(text, "{{user:id}}", GetIdentifierOf(Role::User));
		replace_all_inplace(text, "{{char1:id}}", GetIdentifierOf(Role::Bot1));
		replace_all_inplace(text, "{{char2:id}}", GetIdentifierOf(Role::Bot2));
		replace_all_inplace(text, "{{char3:id}}", GetIdentifierOf(Role::Bot3));
		replace_all_inplace(text, "{{char4:id}}", GetIdentifierOf(Role::Bot4));
		replace_all_inplace(text, "{{char5:id}}", GetIdentifierOf(Role::Bot5));
		replace_all_inplace(text, "{{char6:id}}", GetIdentifierOf(Role::Bot6));
		replace_all_inplace(text, "{{char7:id}}", GetIdentifierOf(Role::Bot7));
		replace_all_inplace(text, "{{char8:id}}", GetIdentifierOf(Role::Bot8));

		replace_all_inplace(text, "{{user:brief}}", GetBriefOf(Role::User));
		replace_all_inplace(text, "{{char1:brief}}", GetBriefOf(Role::Bot1));
		replace_all_inplace(text, "{{char2:brief}}", GetBriefOf(Role::Bot2));
		replace_all_inplace(text, "{{char3:brief}}", GetBriefOf(Role::Bot3));
		replace_all_inplace(text, "{{char4:brief}}", GetBriefOf(Role::Bot4));
		replace_all_inplace(text, "{{char5:brief}}", GetBriefOf(Role::Bot5));
		replace_all_inplace(text, "{{char6:brief}}", GetBriefOf(Role::Bot6));
		replace_all_inplace(text, "{{char7:brief}}", GetBriefOf(Role::Bot7));
		replace_all_inplace(text, "{{char8:brief}}", GetBriefOf(Role::Bot8));

		return text;
	}

	fig::string ChatSession::ApplyNames(fig::string text, Role characterRole) const
	{
		replace_all_inplace(text, "{{char}}", GetNameOf(characterRole));
		return ApplyNames(text);
	}

	fig::string ChatSession::GetSystemPrompt() const
	{
		fig::string prompt;
		if (IsGroupChat())
		{
			prompt = _system_prompt_group;
			replace_all_inplace(prompt, "##FORMATTING##", _formatting_group);
		}
		else
		{
			prompt = _system_prompt_solo;
			replace_all_inplace(prompt, "##FORMATTING##", _formatting_solo);
		}

		replace_all_inplace(prompt, "##STATE_FORMATTING##", _options.flags.IsSet(ChatOptions::Flag::StateVariables) ? _formatting_state : "");
		replace_all_inplace(prompt, "##UNCENSOR_INSTRUCTIONS##", _options.flags.IsSet(ChatOptions::Flag::Uncensored) ? _system_prompt_uncensored : "");
		prompt = trim(prompt);

		if (IsGroupChat())
		{
			prompt.append("\n\n# Characters");

			if (_options.flags.IsSet(ChatOptions::Flag::UseCharacterIds))
			{
				prompt.append("\n{\n");
				// Bots
				for (auto& kvp : _characters)
				{
					auto& character = kvp.second;
					if (is_bot(kvp.first))
					{
						prompt.append(std::format("\t\"@{0}\": {{\"name\": \"{1}\"", ucase(character.characterId), character.shortName));
						if (!empty_or_whitespace(character.brief))
							prompt.append(std::format(", \"info\": \"{0}\"", character.brief));
						prompt.append("}},\n");
					}
				}

				// User
				if (auto user = GetCharacter(Role::User))
				{
					prompt.append(std::format("\t\"@USR\": {{\"name\": \"{0}\"", user.value().shortName));
					if (!empty_or_whitespace(user.value().brief))
						prompt.append(std::format(", \"info\": \"{0}\"", user.value().brief));
					prompt.append("}\n");
				}
				prompt.append("}");
			}
			else
			{
				// Bots
				for (auto& kvp : _characters)
				{
					auto& character = kvp.second;
					if (is_bot(kvp.first))
					{
						prompt.append(std::format("\n- {}", character.shortName));
						if (!empty_or_whitespace(character.brief))
							prompt.append(std::format(": {}", character.brief));
					}
				}

				// User
				if (auto user = GetCharacter(Role::User))
				{
					prompt.append(std::format("\n- {}", user.value().shortName));
					if (!empty_or_whitespace(user.value().brief))
						prompt.append(std::format(": {}", user.value().brief));
				}
			}
		}
		return ApplyNames(prompt);
	}

	fig::string ChatSession::GetDirectorPrompt() const
	{
		fig::string prompt = _formatting_director;
		return ApplyNames(prompt);
	}

	size_t ChatSession::GetBotCount() const
	{
		return std::count_if(_characters.begin(), _characters.end(), [](auto kvp) { return is_bot(kvp.first); });
	}

	fig::string ChatSession::GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const
	{
		fig::string pattern;
		int32_t botCount = (int32_t)GetBotCount();
		for (int i = 0; i < botCount; ++i)
		{
			if (i > 0)
				pattern += "| ";
			if (useCharacterIds)
				pattern += std::format("| \"@{}\"", GetIdentifierOf(bot_from_index(i)));
			else
				pattern += std::format("| \"{}\"", GetNameOf(bot_from_index(i)));
		}
		if (bIncludeUser)
		{
			if (useCharacterIds)
				pattern += std::format("| \"@{}\"", GetIdentifierOf(Role::User));
			else
				pattern += std::format("| \"{}\"", GetNameOf(Role::User));
		}
		return pattern;
	}
} // namespace
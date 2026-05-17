#include <pch.h>
#include "chat/ChatStaging.h"
#include "app/AppState.h"
#include "io/FileUtility.h"
#include "llm/LLMUtility.h"
#include "gui/GUIUtility.h"
#include "gui/Window.h"
#include "gui/OldCharacterImageStore.h"
#include "io/data/CharacterData.h"

#include <exception>
#include <cassert>
#include <format>

using namespace fig::io;
using namespace fig::gui;

namespace fig::chat
{
	static bool s_bInitialized = false;
	static fig::string s_system_prompt_solo;
	static fig::string s_system_prompt_group;
	static fig::string s_system_prompt_character;
	static fig::string s_system_prompt_user;
	static fig::string s_system_prompt_uncensored;
	static fig::string s_formatting_solo;
	static fig::string s_formatting_group;
	static fig::string s_formatting_director;
	static fig::string s_formatting_state;
	
	static bool Initialize()
	{
		if (s_bInitialized)
			return true;

		// System prompt
		s_system_prompt_solo = ReadTextFile("./resources/prompting/prompt_system_solo.txt").value_or("");
		s_system_prompt_group = ReadTextFile("./resources/prompting/prompt_system_group.txt").value_or("");

		// System prompt (character)
		s_system_prompt_character = ReadTextFile("./resources/prompting/prompt_system_character.txt").value_or("");

		// System prompt (user)
		s_system_prompt_user = ReadTextFile("./resources/prompting/prompt_system_user.txt").value_or("");

		// Formatting spec
		s_formatting_solo = ReadTextFile("./resources/prompting/prompt_formatting_solo.txt").value_or("");
		s_formatting_group = ReadTextFile("./resources/prompting/prompt_formatting_group.txt").value_or("");

		// State tracking
		s_formatting_state = ReadTextFile("./resources/prompting/prompt_formatting_state.txt").value_or("");

		// Director prompt
		s_formatting_director = ReadTextFile("./resources/prompting/prompt_formatting_director.txt").value_or("");

		// (Optional) Uncensored instructions
		s_system_prompt_uncensored = ReadTextFile("./resources/prompting/prompt_system_uncensored.txt").value_or("");

		return !s_system_prompt_solo.empty()
			&& !s_system_prompt_character.empty()
			&& !s_system_prompt_user.empty();
	}

	ChatStaging::ChatStaging(ChatOptions options) :
		_options { options }
	{
		if (!Initialize())
			throw std::runtime_error("Failed to initialize chat staging.");
	}

	bool ChatStaging::AssignRole(Role role, const CharacterData& characterData)
	{
		auto& character = _characters[role];
		character = characterData;
		if (role == Role::User)
			character.chatId = "USR";
		if (is_bot(role))
			_numBots = static_cast<int32_t>(std::count_if(_characters.begin(), _characters.end(), [](auto& kvp) { return is_bot(kvp.first); }));
		return true;
	}

	std::optional<CharacterData> ChatStaging::GetCharacter(Role role) const noexcept
	{
		auto itFind = _characters.find(role);
		if (itFind != _characters.end())
			return itFind->second;
		return std::nullopt;
	}

	std::optional<CharacterData> ChatStaging::GetCharacterByChatId(const fig::string& identifier) const noexcept
	{
		if (identifier.empty() || _characters.empty())
			return std::nullopt;

		auto itFind = std::find_if(_characters.begin(), _characters.end(), [identifier](const auto& kvp) {
			return equals(kvp.second.chatId, identifier, true);
		});
		if (itFind != _characters.end())
			return itFind->second;
		return std::nullopt;
	}

	std::optional<CharacterData> ChatStaging::GetCharacterById(const fig::uuid& id) const noexcept
	{
		if (_characters.empty())
			return std::nullopt;

		auto itFind = std::find_if(_characters.begin(), _characters.end(), [&id](const auto& kvp) { return kvp.second.assetId == id; });
		if (itFind != _characters.end())
			return itFind->second;
		return std::nullopt;
	}

	std::optional<CharacterData> ChatStaging::GetCharacterByName(const fig::string& name) const noexcept
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

	Role ChatStaging::GetRoleOf(const fig::string& characterId) const
	{
		if (characterId.empty() || _characters.empty())
			return Role::Undefined;

		auto itFind = std::find_if(_characters.begin(), _characters.end(), [characterId](const auto& kvp) {
			return equals(kvp.second.chatId, characterId, true) || equals(kvp.second.shortName, characterId, true);
		});
		if (itFind != _characters.end())
			return itFind->first;
		return Role::Undefined;
	}

	fig::string ChatStaging::GetChatIdOf(Role role) const
	{
		if (role == Role::User)
			return "USR";
		auto optCharacter = GetCharacter(role);
		if (optCharacter.has_value())
			return ucase(optCharacter.value().chatId);
		return "_UNK";
	}

	fig::string ChatStaging::GetNameOf(Role role) const
	{
		if (role == Role::System)
			return fig::string { Constants::Chat::Names::System };
		if (role == Role::Narrator)
			return fig::string { Constants::Chat::Names::Narrator };
		if (role == Role::Director)
			return fig::string { Constants::Chat::Names::Director };

		auto optCharacter = GetCharacter(role);
		if (optCharacter.has_value())
			return optCharacter.value().shortName;

		return fig::string { Constants::Chat::Names::Unknown };
	}

	fig::gui::ColorPair ChatStaging::GetColorsOf(Role role) const
	{
		if (auto character = GetCharacter(role))
		{
			if (character.value().bgColor.IsDefined() && character.value().borderColor.IsDefined())
			{
				return ColorPair {
					.foreground = character.value().borderColor,
					.background = character.value().bgColor,
				};
			}
		}

		if (is_bot(role))
		{
			return ColorPair {
				.foreground = Colors::DefaultBotMessageBorders[get_bot_index(role) % 8],
				.background = Colors::DefaultBotMessageBackgrounds[get_bot_index(role) % 8],
			};
		}
		else if (role == Role::User)
		{
			return ColorPair {
				.foreground = Colors::DefaultUserMessageBorder,
				.background = Colors::DefaultUserMessageBackground,
			};
		}
		else if (role == Role::System)
		{
			return ColorPair {
				.foreground = Colors::MessageBorderNavy,
				.background = Colors::MessageBackgroundNavy,
			};
		}
		else
		{
			return ColorPair {
				.foreground = Colors::MessageBorderDefault,
				.background = Colors::MessageBackgroundDefault,
			};
		}
	}

	fig::string ChatStaging::GetBriefOf(Role role) const
	{
		auto optCharacter = GetCharacter(role);
		if (!optCharacter.has_value())
			return "";

		fig::string brief = trim(optCharacter.value().brief);
		replace_all_inplace(brief, "{{user}}", GetNameOf(Role::User));
		replace_all_inplace(brief, "{{char}}", optCharacter.value().shortName);
		return brief;
	}

	fig::string ChatStaging::GetPersonaOf(Role role) const
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
			fig::string prompt = s_system_prompt_user;
			replace_all_inplace(prompt, "##PERSONA##", description);
			return ApplyNames(prompt);
		}
		else
		{
			fig::string prompt = s_system_prompt_character;
			replace_all_inplace(prompt, "##PERSONA##", description);
			replace_all_inplace(prompt, "{{user}}", GetNameOf(Role::User));
			replace_all_inplace(prompt, "{{char}}", optCharacter.value().shortName);
			return prompt;
		}
	}

	fig::string ChatStaging::ApplyNames(const fig::string& text) const
	{
		fig::string s = text;
		replace_all_inplace(s, "{{user}}", GetNameOf(Role::User));
		replace_all_inplace(s, "{{char}}", GetNameOf(Role::Bot1));
		replace_all_inplace(s, "{{user:name}}", GetNameOf(Role::User));
		replace_all_inplace(s, "{{char:name}}", GetNameOf(Role::Bot1));

		replace_all_inplace(s, "{{char1:name}}", GetNameOf(Role::Bot1));
		replace_all_inplace(s, "{{char2:name}}", GetNameOf(Role::Bot2));
		replace_all_inplace(s, "{{char3:name}}", GetNameOf(Role::Bot3));
		replace_all_inplace(s, "{{char4:name}}", GetNameOf(Role::Bot4));
		replace_all_inplace(s, "{{char5:name}}", GetNameOf(Role::Bot5));
		replace_all_inplace(s, "{{char6:name}}", GetNameOf(Role::Bot6));
		replace_all_inplace(s, "{{char7:name}}", GetNameOf(Role::Bot7));
		replace_all_inplace(s, "{{char8:name}}", GetNameOf(Role::Bot8));

		replace_all_inplace(s, "{{user:id}}", GetChatIdOf(Role::User));
		replace_all_inplace(s, "{{char1:id}}", GetChatIdOf(Role::Bot1));
		replace_all_inplace(s, "{{char2:id}}", GetChatIdOf(Role::Bot2));
		replace_all_inplace(s, "{{char3:id}}", GetChatIdOf(Role::Bot3));
		replace_all_inplace(s, "{{char4:id}}", GetChatIdOf(Role::Bot4));
		replace_all_inplace(s, "{{char5:id}}", GetChatIdOf(Role::Bot5));
		replace_all_inplace(s, "{{char6:id}}", GetChatIdOf(Role::Bot6));
		replace_all_inplace(s, "{{char7:id}}", GetChatIdOf(Role::Bot7));
		replace_all_inplace(s, "{{char8:id}}", GetChatIdOf(Role::Bot8));

		replace_all_inplace(s, "{{user:brief}}", GetBriefOf(Role::User));
		replace_all_inplace(s, "{{char1:brief}}", GetBriefOf(Role::Bot1));
		replace_all_inplace(s, "{{char2:brief}}", GetBriefOf(Role::Bot2));
		replace_all_inplace(s, "{{char3:brief}}", GetBriefOf(Role::Bot3));
		replace_all_inplace(s, "{{char4:brief}}", GetBriefOf(Role::Bot4));
		replace_all_inplace(s, "{{char5:brief}}", GetBriefOf(Role::Bot5));
		replace_all_inplace(s, "{{char6:brief}}", GetBriefOf(Role::Bot6));
		replace_all_inplace(s, "{{char7:brief}}", GetBriefOf(Role::Bot7));
		replace_all_inplace(s, "{{char8:brief}}", GetBriefOf(Role::Bot8));

		return s;
	}

	fig::string ChatStaging::ApplyNames(const fig::string& text, Role characterRole) const
	{
		fig::string s = text;
		replace_all_inplace(s, "{{char}}", GetNameOf(characterRole));
		return ApplyNames(s);
	}

	fig::string ChatStaging::GetSystemPrompt() const
	{
		fig::string prompt;
		if (GetBotCount() > 1)
		{
			prompt = s_system_prompt_group;
			replace_all_inplace(prompt, "##FORMATTING##", s_formatting_group);
		}
		else
		{
			prompt = s_system_prompt_solo;
			replace_all_inplace(prompt, "##FORMATTING##", s_formatting_solo);
		}

		replace_all_inplace(prompt, "##STATE_FORMATTING##", _options.flags.IsSet(ChatOptions::Flag::StateVariables) ? s_formatting_state : "");
		replace_all_inplace(prompt, "##UNCENSOR_INSTRUCTIONS##", _options.flags.IsSet(ChatOptions::Flag::Uncensored) ? s_system_prompt_uncensored : "");
		prompt = trim(prompt);

		if (GetBotCount() > 1)
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
						prompt.append(std::format("\t\"@{0}\": {{\"name\": \"{1}\"", ucase(character.chatId), character.shortName));
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

	fig::string ChatStaging::GetDirectorPrompt() const
	{
		fig::string prompt = s_formatting_director;
		return ApplyNames(prompt);
	}

	fig::string ChatStaging::GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const
	{
		fig::string pattern;
		int32_t botCount = (int32_t)GetBotCount();
		for (int i = 0; i < botCount; ++i)
		{
			if (i > 0)
				pattern += "| ";
			if (useCharacterIds)
				pattern += std::format("| \"@{}\"", GetChatIdOf(bot_from_index(i)));
			else
				pattern += std::format("| \"{}\"", GetNameOf(bot_from_index(i)));
		}
		if (bIncludeUser)
		{
			if (useCharacterIds)
				pattern += std::format("| \"@{}\"", GetChatIdOf(Role::User));
			else
				pattern += std::format("| \"{}\"", GetNameOf(Role::User));
		}
		return pattern;
	}
} // namespace
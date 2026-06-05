#include <pch.h>
#include "chat/ChatStaging.h"
#include "app/AppState.h"
#include "io/FileUtility.h"
#include "llm/LLMUtility.h"
#include "gui/GUIUtility.h"
#include "gui/Window.h"
#include "gui/OldCharacterImageStore.h"
#include "data/CharacterData.h"

#include <exception>
#include <cassert>
#include <format>

using namespace fig::io;
using namespace fig::data;
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

	bool ChatStaging::AddCharacter(const fig::uuid& in_characterId, Role role, const CharacterData& data)
	{
		fig::uuid characterId = in_characterId;
		if (characterId.empty())
			characterId = GenerateUUID();

		if (_charactersByRole.contains(role))
			return false; // Role already assigned
		if (_charactersByID.contains(characterId))
			return false; // Character already added

		size_t index = _characters.size();

		_characters.push_back(data);
		auto& character = _characters.back();
		if (role == Role::User)
			character.chatId = "USR";
		else if (role == Role::Bot1)
			character.chatId = "AI"; //! @id

		_charactersByID[characterId] = index;
		_charactersByRole[role] = index;

		_bDirtyContext = true;
		return true;
	}

	int32_t ChatStaging::GetBotCount() const noexcept
	{
		return static_cast<int32_t>(std::ranges::count_if(_charactersByRole, [](auto& kvp) { return is_bot(kvp.first); }));
	}

	std::optional<CharacterDataCRef> ChatStaging::GetCharacterByRole(Role role) const noexcept
	{
		if (auto itFind = _charactersByRole.find(role); itFind != _charactersByRole.cend())
			return std::cref(_characters[itFind->second]);
		return std::nullopt;
	}

	std::optional<fig::uuid> ChatStaging::GetCharacterIdByRole(Role role) const noexcept
	{
		if (auto itFind = _charactersByRole.find(role); itFind != _charactersByRole.cend())
			return find_key(_charactersByID, itFind->second);
		return std::nullopt;
	}

	std::optional<CharacterDataCRef> ChatStaging::GetCharacterById(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _charactersByID.find(id); itFind != _charactersByID.cend())
			return std::cref(_characters[itFind->second]);
		return std::nullopt;
	}

	std::optional<CharacterDataCRef> ChatStaging::GetCharacterByChatId(const fig::string& identifier) const noexcept
	{
		if (identifier.empty() || _characters.empty())
			return std::nullopt;

		if (auto itFind = std::ranges::find_if(_characters, [&identifier](auto& character) { return equals(character.chatId, identifier, true); }); itFind != _characters.cend())
			return std::cref(*itFind);
		return std::nullopt;
	}

	std::optional<CharacterDataCRef> ChatStaging::GetCharacterByName(const fig::string& name) const noexcept
	{
		if (name.empty() || _characters.empty())
			return std::nullopt;

		if (auto itFind = std::ranges::find_if(_characters, [&name](auto& character) { return equals(character.shortName, name, true); }); itFind != _characters.cend())
			return std::cref(*itFind);
		return std::nullopt;
	}

	Role ChatStaging::GetRoleOf(const fig::string& characterId) const
	{
		if (characterId.empty() || _characters.empty())
			return Role::Undefined;

		if (auto itFind = std::ranges::find_if(_charactersByRole, [this, characterId](const auto& kvp) { return equals(_characters[kvp.second].chatId, characterId, true) || equals(_characters[kvp.second].shortName, characterId, true);}); itFind != _charactersByRole.cend())
			return itFind->first;
		return Role::Undefined;
	}

	fig::string ChatStaging::GetChatIdOf(Role role) const
	{
		if (role == Role::User)
			return "USR";
		if (auto try_find = GetCharacterByRole(role))
			return ucase((*try_find).get().chatId);
		return "UNK?";
	}

	fig::string ChatStaging::GetNameOf(Role role) const
	{
		if (role == Role::System)
			return fig::string { Constants::Chat::Names::System };
		if (role == Role::Narrator)
			return fig::string { Constants::Chat::Names::Narrator };
		if (role == Role::Director)
			return fig::string { Constants::Chat::Names::Director };

		if (auto try_find = GetCharacterByRole(role))
			return (*try_find).get().shortName;

		return fig::string { Constants::Chat::Names::Unknown };
	}

	fig::gui::ColorPair ChatStaging::GetColorsOf(Role role) const
	{
		if (auto try_character = GetCharacterByRole(role))
		{
			auto& character = (*try_character).get();
			if (character.bgColor.IsDefined() && character.borderColor.IsDefined())
			{
				return ColorPair {
					.foreground = character.borderColor,
					.background = character.bgColor,
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
		if (auto try_character = GetCharacterByRole(role))
		{
			auto& character = (*try_character).get();

			fig::string brief = trim(character.brief);
			replace_all_inplace(brief, "{{user}}", GetNameOf(Role::User));
			replace_all_inplace(brief, "{{char}}", character.shortName);
			return brief;
		}
		return "";
	}

	fig::string ChatStaging::GetPersonaOf(Role role) const
	{
		if(auto try_character = GetCharacterByRole(role))
		{
			auto& character = (*try_character).get();
			fig::string persona = trim(character.GetAttribute(Constants::CharacterAttributes::Persona).value_or(""));
			if (persona.empty())
				return "";

			// Format
			if (role == Role::User)
			{
				fig::string prompt = s_system_prompt_user;
				replace_all_inplace(prompt, "##PERSONA##", persona);
				return ApplyNames(prompt);
			}
			else
			{
				fig::string prompt = s_system_prompt_character;
				replace_all_inplace(prompt, "##PERSONA##", persona);
				replace_all_inplace(prompt, "{{user}}", GetNameOf(Role::User));
				replace_all_inplace(prompt, "{{char}}", character.shortName);
				return prompt;
			}
		}
		return "";
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
				for (auto [role, idx] : _charactersByRole)
				{
					auto& character = _characters[idx];
					if (is_bot(role))
					{
						prompt.append(std::format("\t\"@{0}\": {{\"name\": \"{1}\"", ucase(character.chatId), character.shortName));
						if (!empty_or_whitespace(character.brief))
							prompt.append(std::format(", \"info\": \"{0}\"", character.brief));
						prompt.append("}},\n");
					}
				}

				// User
				if (auto try_user = GetCharacterByRole(Role::User))
				{
					auto& user = (*try_user).get();
					prompt.append(std::format("\t\"@USR\": {{\"name\": \"{0}\"", user.shortName));
					if (!empty_or_whitespace(user.brief))
						prompt.append(std::format(", \"info\": \"{0}\"", user.brief));
					prompt.append("}\n");
				}
				prompt.append("}");
			}
			else
			{
				// Bots
				for (auto [role, idx] : _charactersByRole)
				{
					auto& character = _characters[idx];
					if (is_bot(role))
					{
						prompt.append(std::format("\n- {}", character.shortName));
						if (!empty_or_whitespace(character.brief))
							prompt.append(std::format(": {}", character.brief));
					}
				}

				// User
				if (auto try_user = GetCharacterByRole(Role::User))
				{
					auto& user = (*try_user).get();
					prompt.append(std::format("\n- {}", user.shortName));
					if (!empty_or_whitespace(user.brief))
						prompt.append(std::format(": {}", user.brief));
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

	fig::uuid ChatStaging::GenerateUUID() const noexcept
	{
		fig::uuid uuid = _CreateUUID();
		while (_charactersByID.contains(uuid))
			uuid = _CreateUUID();
		return uuid;
	}

	const Context& ChatStaging::GetContext() noexcept
	{
		if (_bDirtyContext)
			UpdateContext();
		return _context;
	}

	void ChatStaging::UpdateContext()
	{
		_context.Clear();
		for (auto& kvp : _charactersByRole)
		{
			auto role = kvp.first;
			auto& character = _characters[kvp.second];
			if (is_bot(role))
			{
				size_t bot_index = get_bot_index(role) + 1;
				auto charKey = std::format("char{}", bot_index);
				_context.AddContext(charKey, character);
				_context.AddContextAlias(std::to_string(bot_index), charKey);
			}
			else if (role == Role::User)
			{
				_context.AddContext("user", character);
				_context.AddContextAlias("u", "user");
				_context.AddValueAlias("user", "user:name");
			}
		}

		if (_charactersByRole.contains(Role::Bot1))
		{
			_context.AddContextAlias("char", "char1");
			_context.AddContextAlias("c", "char1");
			_context.AddValueAlias("char", "char1:name");
		}

		_bDirtyContext = false;
	}
} // namespace
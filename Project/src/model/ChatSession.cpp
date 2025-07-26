#include "model/ChatSession.h"
#include "llm/LLMUtility.h"
#include "util/Common.h"
#include "util/StringUtility.h"

#include <exception>

bool ChatSession::Initialize()
{
	// System prompt (common)
	_system_prompt_common = ReadTextFile("./resources/prompting/prompt_system_common.txt").value_or("");

	// System prompt (character)
	_system_prompt_character = ReadTextFile("./resources/prompting/prompt_system_character.txt").value_or("");

	// System prompt (user)
	_system_prompt_user = ReadTextFile("./resources/prompting/prompt_system_user.txt").value_or("");

	// Formatting spec
	_formatting_common = ReadTextFile("./resources/prompting/prompt_formatting_common.txt").value_or("");

	// Director prompt
	_formatting_director = ReadTextFile("./resources/prompting/prompt_formatting_director.txt").value_or("");

	return !_system_prompt_common.empty()
		&& !_system_prompt_character.empty()
		&& !_system_prompt_user.empty();
}

bool ChatSession::LoadCharacter(Role role, string filename)
{
	Character character;
	if (character.LoadFromXml(filename))
	{
		_characters[role] = std::move(character);
		return true;
	}
	return false;
}

std::optional<Character> ChatSession::GetCharacter(Role role) const
{
	auto itFind = _characters.find(role);
	if (itFind != std::cend(_characters))
		return itFind->second;
	return std::nullopt;
}

string ChatSession::GetNameOf(Role role) const
{
	if (role == Role::System)
		return "system";
	if (role == Role::Narrator)
		return "narrator";
	if (role == Role::Director)
		return "director";
	auto optCharacter = GetCharacter(role);
	if (optCharacter.has_value())
		return optCharacter.value().name;
	return "Unknown";
}

string ChatSession::GetPersonaOf(Role role) const
{
	auto optCharacter = GetCharacter(role);
	if (!optCharacter.has_value())
		return "";

	string description = optCharacter.value().description;
	if (description.empty())
		return "";

	if (role == Role::User)
	{
		string prompt = _system_prompt_user;
		string_util::replace_all(prompt, "##PERSONA##", description);
		return ApplyNames(prompt);
	}
	else
	{
		string prompt = _system_prompt_character;
		string_util::replace_all(prompt, "##PERSONA##", description);
		return ApplyNames(prompt, role);
	}
}

string ChatSession::ApplyNames(string text) const
{
	string_util::replace_all(text, "{{user}}", GetNameOf(Role::User));
	string_util::replace_all(text, "{{char}}", GetNameOf(Role::Bot1));

	string_util::replace_all(text, "{{char1}}", GetNameOf(Role::Bot1));
	string_util::replace_all(text, "{{char2}}", GetNameOf(Role::Bot2));
	string_util::replace_all(text, "{{char3}}", GetNameOf(Role::Bot3));
	string_util::replace_all(text, "{{char4}}", GetNameOf(Role::Bot4));
	string_util::replace_all(text, "{{char5}}", GetNameOf(Role::Bot5));
	string_util::replace_all(text, "{{char6}}", GetNameOf(Role::Bot6));
	string_util::replace_all(text, "{{char7}}", GetNameOf(Role::Bot7));
	string_util::replace_all(text, "{{char8}}", GetNameOf(Role::Bot8));
	return text;
}

string ChatSession::ApplyNames(string text, Role characterRole) const
{
	string_util::replace_all(text, "{{char}}", GetNameOf(characterRole));
	return ApplyNames(text);
}

string ChatSession::GetSystemPrompt() const
{
	string prompt = _system_prompt_common;
	string_util::replace_all(prompt, "##FORMATTING_SPEC##", _formatting_common);
	return ApplyNames(prompt);
}

string ChatSession::GetDirectorPrompt() const
{
	string prompt = _formatting_director;
	return ApplyNames(prompt);
}

size_t ChatSession::GetBotCount() const
{
	return std::count_if(std::begin(_characters), std::end(_characters), [](auto kvp) { return is_bot(kvp.first); });
}
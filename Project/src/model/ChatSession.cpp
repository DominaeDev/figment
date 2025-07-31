#include "model/ChatSession.h"

#include "util/Common.h"
#include "util/StringUtility.h"
#include "llm/LLMUtility.h"
#include "gui/CharacterImageStore.h"
#include "gui/Color.h"

#include <exception>
#include <cassert>

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
		if (!string_util::empty_or_whitespace(character.portraitFilename))
			CharacterImageStore::LoadCharacterPortrait(character.id, "./characters/" + character.portraitFilename);

		_characters[role] = std::move(character);
		return true;
	}
	return false;
}

std::optional<Character> ChatSession::GetCharacter(Role role) const
{
	auto itFind = _characters.find(role);
	if (itFind != std::end(_characters))
		return itFind->second;
	return std::nullopt;
}

std::optional<Character> ChatSession::GetCharacterById(string identifier) const
{
	if (identifier.empty() || _characters.empty())
		return std::nullopt;
	
	auto itFind = std::find_if(std::begin(_characters), std::end(_characters), [identifier](const auto& kvp) {
		return string_util::equals(kvp.second.id, identifier, true);
	});
	if (itFind != std::end(_characters))
		return itFind->second;
	return std::nullopt;
}

std::optional<Character> ChatSession::GetCharacterByName(string name) const
{
	if (name.empty() || _characters.empty())
		return std::nullopt;
	
	auto itFind = std::find_if(std::begin(_characters), std::end(_characters), [name](const auto& kvp) {
		return string_util::equals(kvp.second.name, name, true);
	});
	if (itFind != std::end(_characters))
		return itFind->second;
	return std::nullopt;
}

Role ChatSession::GetRoleOf(string characterId) const
{
	if (characterId.empty() || _characters.empty())
		return Role::Undefined;
	
	auto itFind = std::find_if(std::begin(_characters), std::end(_characters), [characterId](const auto& kvp) {
		return string_util::equals(kvp.second.id, characterId, true);
	});
	if (itFind != std::end(_characters))
		return itFind->first;
	return Role::Undefined;
}

string ChatSession::GetIdentifierOf(Role role) const
{
	if (role == Role::User)
		return "USR";
	auto optCharacter = GetCharacter(role);
	if (optCharacter.has_value())
		return string_util::ucase(optCharacter.value().id);
	return "_UNK";
}

string ChatSession::GetNameOf(Role role) const
{
	if (role == Role::System)
		return "system";
	if (role == Role::Narrator)
		return "Narrator";
	if (role == Role::Director)
		return "Director";
	auto optCharacter = GetCharacter(role);
	if (optCharacter.has_value())
		return optCharacter.value().name;
	return "Unknown";
}

std::pair<Color, Color> ChatSession::GetColorsOf(Role role) const
{
	if (auto character = GetCharacter(role))
	{
		if (color_util::is_defined(character.value().bgColor) && color_util::is_defined(character.value().borderColor))
			return std::make_pair(character.value().bgColor, character.value().borderColor);
	}

	if (is_bot(role))
		return std::make_pair(Colors::DefaultBotMessageBackgrounds[get_bot_index(role) % 8], Colors::DefaultBotMessageBorders[get_bot_index(role) % 8]);
	else if (role == Role::User)
		return std::make_pair(Colors::DefaultUserMessageBackground, Colors::DefaultUserMessageBorder);
	else
		return std::make_pair(Colors::MessageBackgroundDefault, Colors::MessageBorderDefault);
}

string ChatSession::GetBriefOf(Role role) const
{
	auto optCharacter = GetCharacter(role);
	if (!optCharacter.has_value())
		return "";

	string brief = string_util::trim(optCharacter.value().brief);
	string_util::replace_all(brief, "{{user}}", GetNameOf(Role::User));
	string_util::replace_all(brief, "{{char}}", optCharacter.value().name);
	return brief;
}

string ChatSession::GetPersonaOf(Role role) const
{
	auto optCharacter = GetCharacter(role);
	if (!optCharacter.has_value())
		return "";

	string description = string_util::trim(optCharacter.value().description);
	if (description.empty())
		return "";

	// Format
	if (role == Role::User)
	{
		string prompt = _system_prompt_user;
		string_util::replace_all(prompt, "##PERSONA##", description);
		string_util::replace_all(prompt, "{{user}}", GetNameOf(Role::User));
		string_util::replace_all(prompt, "{{char}}", optCharacter.value().name);
		return prompt;
	}
	else
	{
		string prompt = _system_prompt_character;
		string_util::replace_all(prompt, "##PERSONA##", description);
		string_util::replace_all(prompt, "{{user}}", GetNameOf(Role::User));
		string_util::replace_all(prompt, "{{char}}", optCharacter.value().name);
		return prompt;
	}
}

string ChatSession::ApplyNames(string text) const
{
	string_util::replace_all(text, "{{user}}", GetNameOf(Role::User));
	string_util::replace_all(text, "{{char}}", GetNameOf(Role::Bot1));
	string_util::replace_all(text, "{{user:name}}", GetNameOf(Role::User));
	string_util::replace_all(text, "{{char:name}}", GetNameOf(Role::Bot1));

	string_util::replace_all(text, "{{char1:name}}", GetNameOf(Role::Bot1));
	string_util::replace_all(text, "{{char2:name}}", GetNameOf(Role::Bot2));
	string_util::replace_all(text, "{{char3:name}}", GetNameOf(Role::Bot3));
	string_util::replace_all(text, "{{char4:name}}", GetNameOf(Role::Bot4));
	string_util::replace_all(text, "{{char5:name}}", GetNameOf(Role::Bot5));
	string_util::replace_all(text, "{{char6:name}}", GetNameOf(Role::Bot6));
	string_util::replace_all(text, "{{char7:name}}", GetNameOf(Role::Bot7));
	string_util::replace_all(text, "{{char8:name}}", GetNameOf(Role::Bot8));

	string_util::replace_all(text, "{{user:id}}", GetIdentifierOf(Role::User));
	string_util::replace_all(text, "{{char1:id}}", GetIdentifierOf(Role::Bot1));
	string_util::replace_all(text, "{{char2:id}}", GetIdentifierOf(Role::Bot2));
	string_util::replace_all(text, "{{char3:id}}", GetIdentifierOf(Role::Bot3));
	string_util::replace_all(text, "{{char4:id}}", GetIdentifierOf(Role::Bot4));
	string_util::replace_all(text, "{{char5:id}}", GetIdentifierOf(Role::Bot5));
	string_util::replace_all(text, "{{char6:id}}", GetIdentifierOf(Role::Bot6));
	string_util::replace_all(text, "{{char7:id}}", GetIdentifierOf(Role::Bot7));
	string_util::replace_all(text, "{{char8:id}}", GetIdentifierOf(Role::Bot8));

	string_util::replace_all(text, "{{user:brief}}", GetBriefOf(Role::User));
	string_util::replace_all(text, "{{char1:brief}}", GetBriefOf(Role::Bot1));
	string_util::replace_all(text, "{{char2:brief}}", GetBriefOf(Role::Bot2));
	string_util::replace_all(text, "{{char3:brief}}", GetBriefOf(Role::Bot3));
	string_util::replace_all(text, "{{char4:brief}}", GetBriefOf(Role::Bot4));
	string_util::replace_all(text, "{{char5:brief}}", GetBriefOf(Role::Bot5));
	string_util::replace_all(text, "{{char6:brief}}", GetBriefOf(Role::Bot6));
	string_util::replace_all(text, "{{char7:brief}}", GetBriefOf(Role::Bot7));
	string_util::replace_all(text, "{{char8:brief}}", GetBriefOf(Role::Bot8));

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
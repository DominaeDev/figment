#pragma once

#include "Types.h"
#include "llm/LLMTypes.h"
#include "model/Character.h"
#include <optional>

class ChatSession
{
public:
	bool Initialize(LLMOptions options);
	bool LoadCharacter(Role role, string filename);

	std::optional<Character> GetCharacter(Role role) const;
	std::optional<Character> GetCharacterById(string characterId) const;
	std::optional<Character> GetCharacterByName(string name) const;

	string GetSystemPrompt() const;
	string GetDirectorPrompt() const;
	string GetIdentifierOf(Role role) const;
	string GetNameOf(Role role) const;
	string GetPersonaOf(Role role) const;
	string GetBriefOf(Role role) const;
	std::pair<Color, Color> GetColorsOf(Role role) const;
	Role GetRoleOf(string characterId) const;
	size_t GetBotCount() const;
	bool IsGroupChat() const { return GetBotCount() > 1; }
	string GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const;

	[[nodiscard]] string ApplyNames(string text) const;
	[[nodiscard]] string ApplyNames(string text, Role characterRole) const;

	const std::map<Role, Character>& GetCharactersByRole() const { return _characters; }

protected:
	std::map<Role, Character> _characters {};
	LLMOptions _options {};

	// Prompts
	string _system_prompt_solo;
	string _system_prompt_group;
	string _system_prompt_character;
	string _system_prompt_user;
	string _system_prompt_uncensored;
	string _formatting_solo;
	string _formatting_group;
	string _formatting_director;
	string _formatting_state;
};
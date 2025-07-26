#pragma once

#include "Types.h"
#include "llm/LLMTypes.h"
#include "model/Character.h"
#include <optional>

class ChatSession
{
public:
	bool Initialize();
	bool LoadCharacter(Role role, string filename);

	std::optional<Character> GetCharacter(Role role) const;

	string GetSystemPrompt() const;
	string GetDirectorPrompt() const;
	string GetNameOf(Role role) const;
	string GetPersonaOf(Role role) const;
	size_t GetBotCount() const;

	[[nodiscard]] string ApplyNames(string text) const;
	[[nodiscard]] string ApplyNames(string text, Role characterRole) const;
protected:
	std::map<Role, Character> _characters {};
	string _system_prompt_common;
	string _system_prompt_character;
	string _system_prompt_user;
	string _formatting_common;
	string _formatting_director;
};
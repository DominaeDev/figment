#pragma once

#include "model/ChatTypes.h"
#include "model/ChatOptions.h"
#include "model/CharacterData.h"
#include <optional>
#include <map>

namespace fig::io
{
	class ChatSession
	{
	public:
		bool Initialize(ChatOptions options);
		bool LoadCharacter(Role role, fig::string filename);

		std::optional<CharacterData> GetCharacter(Role role) const;
		std::optional<CharacterData> GetCharacterById(fig::string characterId) const;
		std::optional<CharacterData> GetCharacterByName(fig::string name) const;

		fig::string GetSystemPrompt() const;
		fig::string GetDirectorPrompt() const;
		fig::string GetIdentifierOf(Role role) const;
		fig::string GetNameOf(Role role) const;
		fig::string GetPersonaOf(Role role) const;
		fig::string GetBriefOf(Role role) const;
		std::pair<fig::gui::Color, fig::gui::Color> GetColorsOf(Role role) const;
		Role GetRoleOf(fig::string characterId) const;
		size_t GetBotCount() const;
		bool IsGroupChat() const { return GetBotCount() > 1; }
		fig::string GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const;

		[[nodiscard]] fig::string ApplyNames(fig::string text) const;
		[[nodiscard]] fig::string ApplyNames(fig::string text, Role characterRole) const;

		const std::map<Role, CharacterData>& GetCharactersByRole() const { return _characters; }

	protected:
		std::map<Role, CharacterData> _characters {};
		ChatOptions _options {};

		// Prompts
		fig::string _system_prompt_solo;
		fig::string _system_prompt_group;
		fig::string _system_prompt_character;
		fig::string _system_prompt_user;
		fig::string _system_prompt_uncensored;
		fig::string _formatting_solo;
		fig::string _formatting_group;
		fig::string _formatting_director;
		fig::string _formatting_state;
	};
}
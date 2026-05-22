#ifndef CHAT_STAGING_H__
#define CHAT_STAGING_H__
#pragma once

#include "chat/ChatTypes.h"
#include "chat/ChatOptions.h"
#include "data/CharacterData.h"

namespace fig::chat
{
	class ChatStaging
	{
	public:
		ChatStaging() = default;
		ChatStaging(ChatOptions options);

		bool AssignRole(Role role, const fig::data::CharacterData& character);

		std::optional<fig::data::CharacterData> GetCharacter(Role role) const noexcept;
		std::optional<fig::data::CharacterData> GetCharacterById(const fig::uuid& id) const noexcept;
		std::optional<fig::data::CharacterData> GetCharacterByChatId(const fig::string& characterId) const noexcept;
		std::optional<fig::data::CharacterData> GetCharacterByName(const fig::string& name) const noexcept;
		const std::map<Role, fig::data::CharacterData>& GetCharacters() const noexcept { return _characters; }

		fig::string GetSystemPrompt() const;
		fig::string GetDirectorPrompt() const;
		fig::string GetPersonaOf(Role role) const;
		fig::string GetBriefOf(Role role) const;
		Role GetRoleOf(const fig::string& characterId) const;
		inline int32_t GetBotCount() const noexcept { return _numBots; }
		inline bool IsGroupChat() const noexcept { return _numBots > 1; }
		
		fig::string GetChatIdOf(Role role) const;
		fig::string GetNameOf(Role role) const;
		fig::string GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const;
		fig::gui::ColorPair GetColorsOf(Role role) const;

		[[nodiscard]] fig::string ApplyNames(const fig::string& text) const;
		[[nodiscard]] fig::string ApplyNames(const fig::string& text, Role characterRole) const;

	private:
		std::map<Role, fig::data::CharacterData> _characters {};
		ChatOptions _options {};
		int32_t _numBots {};
	};
}

#endif
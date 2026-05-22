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

		bool AddCharacter(const fig::uuid& characterId, Role role, const fig::data::CharacterData& data);

		std::optional<fig::uuid> GetCharacterIdByRole(Role role) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterByRole(Role role) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterById(const fig::uuid& id) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterByChatId(const fig::string& characterId) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterByName(const fig::string& name) const noexcept;
		const std::vector<fig::data::CharacterData>& GetCharacters() const noexcept { return _characters; }

		fig::string GetSystemPrompt() const;
		fig::string GetDirectorPrompt() const;
		fig::string GetPersonaOf(Role role) const;
		fig::string GetBriefOf(Role role) const;
		Role GetRoleOf(const fig::string& characterId) const;
		int32_t GetBotCount() const noexcept;
		inline bool IsGroupChat() const noexcept { return GetBotCount() > 1; }
		
		fig::string GetChatIdOf(Role role) const;
		fig::string GetNameOf(Role role) const;
		fig::string GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const;
		fig::gui::ColorPair GetColorsOf(Role role) const;

		[[nodiscard]] fig::string ApplyNames(const fig::string& text) const;
		[[nodiscard]] fig::string ApplyNames(const fig::string& text, Role characterRole) const;

	private:
		std::vector<fig::data::CharacterData> _characters {};
		std::map<fig::uuid, fig::data::CharacterData*> _charactersByID {};
		std::map<Role, fig::data::CharacterData*> _charactersByRole {};
		ChatOptions _options {};
	};
}

#endif
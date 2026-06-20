#ifndef CHAT_STAGING_H__
#define CHAT_STAGING_H__
#pragma once

#include "chat/ChatTypes.h"
#include "chat/ChatOptions.h"
#include "data/CharacterData.h"
#include "chat/PromptScaffold.h"

namespace fig::chat
{
	class ChatStaging
	{
	public:
		ChatStaging() = default;
		ChatStaging(const PromptScaffold& scaffold, ChatOptions options);
		ChatStaging(PromptScaffold&& scaffold, ChatOptions options);

		bool AddCharacter(const fig::uuid& characterId, Role role, const fig::data::CharacterData& data);
		bool HasCharacter(Role role) const noexcept { return _charactersByRole.contains(role); }
		bool HasCharacter(fig::uuid id) const noexcept { return _charactersByID.contains(id); }

		std::optional<fig::uuid> GetCharacterIdByRole(Role role) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterByRole(Role role) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterById(const fig::uuid& id) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterByChatId(const fig::string& characterId) const noexcept;
		std::optional<fig::data::CharacterDataCRef> GetCharacterByName(const fig::string& name) const noexcept;
		const std::vector<fig::data::CharacterData>& GetCharacters() const noexcept { return _characters; }

		std::vector<PromptBlock> GetPromptBlocks();
		fig::string GetPersonaOf(Role role);
		fig::string GetBriefOf(Role role);
		Role GetRoleOf(const fig::string& characterId) const;
		int32_t GetBotCount() const noexcept;
		inline bool IsGroupChat() const noexcept { return GetBotCount() > 1; }

		const fig::string& GetGrammar() const;
		
		fig::string GetChatIdOf(Role role) const;
		fig::string GetNameOf(Role role) const;
		fig::gui::ColorPair GetColorsOf(Role role) const;

		[[nodiscard]] inline Context& GetContext() noexcept { return GetContext(Role::Bot1); }
		[[nodiscard]] Context& GetContext(Role primaryRole) noexcept;

	private:
		Context& GetContext_Internal() noexcept;

		fig::uuid GenerateUUID() const noexcept;
		void UpdateContext();

		std::vector<fig::data::CharacterData> _characters {};
		std::map<fig::uuid, size_t> _charactersByID {};
		std::map<Role, size_t> _charactersByRole {};
		PromptScaffold _promptScaffold;

		ChatOptions _options {};
		Context _context {};
		bool _bDirtyContext {};
	};
}

#endif
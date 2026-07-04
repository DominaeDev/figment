#pragma once

#include "chat/ChatTypes.h"
#include "chat/ChatOptions.h"
#include "data/Character.h"
#include "data/Scenario.h"
#include "chat/PromptScaffold.h"

namespace fig::chat
{
	class ChatStaging
	{
	public:
		ChatStaging() = default;
		ChatStaging(const fig::data::Scenario& scenario, const PromptScaffold& scaffold, ChatOptions options);
		ChatStaging(fig::data::Scenario&& scenario, PromptScaffold&& scaffold, ChatOptions options);

		bool AddCharacter(const fig::uuid& characterId, Role role, const fig::data::Character& data);
		bool HasCharacter(Role role) const noexcept { return _charactersByRole.contains(role); }
		bool HasCharacter(fig::uuid id) const noexcept { return _charactersByID.contains(id); }

		std::optional<fig::uuid> GetCharacterIdByRole(Role role) const noexcept;
		fig::optional_cref<fig::data::Character> GetCharacterByRole(Role role) const noexcept;
		fig::optional_cref<fig::data::Character> GetCharacterByRole(fig::handle handle) const noexcept;
		fig::optional_cref<fig::data::Character> GetCharacterById(const fig::uuid& id) const noexcept;
		fig::optional_cref<fig::data::Character> GetCharacterByChatId(const fig::string& characterId) const noexcept;
		fig::optional_cref<fig::data::Character> GetCharacterByName(const fig::string& name) const noexcept;
		const std::vector<fig::data::Character>& GetCharacters() const noexcept { return _characters; }
		const PromptScaffold& GetPromptScaffold() const noexcept { return _promptScaffold; }
		const fig::data::Scenario& GetScenario() const noexcept { return _scenario; }

		std::vector<PromptBlock> GetStagingBlocks();
		fig::string GetPersonaOf(Role role);
		fig::string GetBriefOf(Role role);
		Role GetRoleOf(const fig::string& characterId) const;
		Role GetRoleFromHandle(const fig::handle& handle) const;
		int32_t GetBotCount() const noexcept;
		bool IsGroupChat() const noexcept { return GetBotCount() > 1; }

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

		std::vector<fig::data::Character> _characters {};
		std::map<fig::uuid, size_t> _charactersByID {};
		std::map<Role, size_t> _charactersByRole {};
		fig::data::Scenario _scenario {};
		PromptScaffold _promptScaffold {};

		ChatOptions _options {};
		Context _context {};
		bool _bDirtyContext {};
	};
}

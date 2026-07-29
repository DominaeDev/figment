#include <pch.h>
#include "chat/ChatStaging.h"
#include "chat/PromptBuilder.h"
#include "app/AppState.h"
#include "io/FileUtility.h"
#include "llm/LLMUtility.h"
#include "gui/GUIUtility.h"
#include "gui/Window.h"
#include "data/Character.h"
#include "text/TextEvaluator.h"

#include <exception>
#include <cassert>

using namespace fig::io;
using namespace fig::data;
using namespace fig::gui;

namespace fig::chat
{
	ChatStaging::ChatStaging(const Scenario& scenario, const PromptScaffold& scaffold, ChatOptions options) :
		_scenario { scenario },
		_promptScaffold { scaffold },
		_options { options }
	{
	}

	ChatStaging::ChatStaging(Scenario&& scenario, PromptScaffold&& scaffold, ChatOptions options) :
		_scenario { std::move(scenario) },
		_promptScaffold { std::move(scaffold) },
		_options { options }
	{
	}

	bool ChatStaging::AddCharacter(const fig::uuid& in_characterId, Role role, const Character& data)
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

	fig::optional_cref<Character> ChatStaging::GetCharacterByRole(Role role) const noexcept
	{
		if (auto itFind = _charactersByRole.find(role); itFind != _charactersByRole.cend())
			return make_optional_cref(_characters[itFind->second]);
		return fig::nullref;
	}

	std::optional<fig::uuid> ChatStaging::GetCharacterIdByRole(Role role) const noexcept
	{
		if (auto itFind = _charactersByRole.find(role); itFind != _charactersByRole.cend())
			return find_key(_charactersByID, itFind->second);
		return std::nullopt;
	}

	fig::optional_cref<Character> ChatStaging::GetCharacterById(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _charactersByID.find(id); itFind != _charactersByID.cend())
			return make_optional_cref(_characters[itFind->second]);
		return fig::nullref;
	}

	fig::optional_cref<Character> ChatStaging::GetCharacterByChatId(const fig::string& identifier) const noexcept
	{
		if (identifier.empty() || _characters.empty())
			return fig::nullref;

		if (auto itFind = std::ranges::find_if(_characters, [&identifier](auto& character) { return equals(character.chatId, identifier, true); }); itFind != _characters.cend())
			return make_optional_cref(*itFind);
		return fig::nullref;
	}

	Role ChatStaging::GetRoleFromHandle(const fig::handle& handle) const
	{
		if (handle == "char" or handle == "character")
			return Role::Bot1;
		else if (handle == "user")
			return Role::User;
		else if (handle == "narrator")
			return Role::Narrator;
		else if (handle == "director")
			return Role::Director;
		else if (handle == "system")
			return Role::System;
		else if (handle == "char1" or handle == "bot1" or handle == "character1")
			return Role::Bot1;
		else if (handle == "char2" or handle == "bot2" or handle == "character2")
			return Role::Bot2;
		else if (handle == "char3" or handle == "bot3" or handle == "character3")
			return Role::Bot3;
		else if (handle == "char4" or handle == "bot4" or handle == "character4")
			return Role::Bot4;

		// Role slots
		auto& slots = _scenario.GetRoleSlots();
		if (auto itSlot = std::ranges::find_if(slots, [&](auto& s) { return s.id == handle; }); itSlot != std::cend(slots))
			return (*itSlot).role;
		return Role::Undefined;
	}

	fig::optional_cref<Character> ChatStaging::GetCharacterByRole(fig::handle handle) const noexcept
	{
		return GetCharacterByRole(GetRoleFromHandle(handle));
	}

	fig::optional_cref<Character> ChatStaging::GetCharacterByName(const fig::string& name) const noexcept
	{
		if (name.empty() || _characters.empty())
			return std::nullopt;

		if (auto itFind = std::ranges::find_if(_characters, [&name](auto& character) { return equals(character.shortName, name, true); }); itFind != _characters.cend())
			return make_optional_cref(*itFind);
		return fig::nullref;
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
			return ucase((*try_find).chatId);
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
			return (*try_find).shortName;

		return fig::string { Constants::Chat::Names::Unknown };
	}

	fig::color_pair ChatStaging::GetColorsOf(Role role) const
	{
		if (auto try_character = GetCharacterByRole(role))
		{
			auto& character = *try_character;
			if (character.bgColor.IsDefined() && character.borderColor.IsDefined())
			{
				return fig::color_pair {
					.foreground = character.borderColor,
					.background = character.bgColor,
				};
			}
		}

		if (is_bot(role))
		{
			return fig::color_pair {
				.foreground = Color::DefaultBotMessageBorders[get_bot_index(role) % 8],
				.background = Color::DefaultBotMessageBackgrounds[get_bot_index(role) % 8],
			};
		}
		else if (role == Role::User)
		{
			return fig::color_pair {
				.foreground = Color::DefaultUserMessageBorder,
				.background = Color::DefaultUserMessageBackground,
			};
		}
		else if (role == Role::System)
		{
			return fig::color_pair {
				.foreground = Color::MessageBorderNavy,
				.background = Color::MessageBackgroundNavy,
			};
		}
		else
		{
			return fig::color_pair {
				.foreground = Color::MessageBorderDefault,
				.background = Color::MessageBackgroundDefault,
			};
		}
	}

	fig::string ChatStaging::GetBriefOf(Role role)
	{
		if (auto try_character = GetCharacterByRole(role))
		{
			auto& character = *try_character;

			fig::string brief = trim(character.brief);
			brief = eval_text(brief, GetContext(role));
			return brief;
		}
		return "";
	}

	fig::string ChatStaging::GetPersonaOf(Role role)
	{
		if (auto try_character = GetCharacterByRole(role))
		{
			auto& character = *try_character;
			fig::string persona = character.GetAttribute(Constants::CharacterAttributes::Persona).value_or("");
			return eval_text(persona, GetContext(role));
		}
		return "";
	}

	std::vector<PromptBlock> ChatStaging::GetStagingBlocks()
	{
		return PromptBuilder::GetStagingBlocks(*this);
	}

	fig::uuid ChatStaging::GenerateUUID() const noexcept
	{
		fig::uuid uuid = _CreateUUID();
		while (_charactersByID.contains(uuid))
			uuid = _CreateUUID();
		return uuid;
	}

	Context& ChatStaging::GetContext(Role primaryRole) noexcept
	{
		if (not is_bot(primaryRole))
			return GetContext(fig::chat::Role::Bot1); // Fallback

		// Update
		auto& ctx = GetContext_Internal();

		// Set primary
		auto primarySelector = ContextSelector::FromRole(primaryRole);
		ctx.SetAlias("current", primarySelector);
		ctx.SetPrimarySelector(primarySelector);
		return ctx;
	}

	void ChatStaging::RefreshContext() noexcept
	{
		GetContext_Internal();
	}

	Context& ChatStaging::GetContext_Internal() noexcept
	{
		if (_bDirtyContext)
			UpdateContext();
		return _context;
	}

	void ChatStaging::UpdateContext()
	{
		_bDirtyContext = false;

		_context.Clear();
		_context.SetMacroProvider(Global::GetMacroProvider());

		// Characters
		for (auto& kvp : _charactersByRole)
		{
			auto role = kvp.first;
			auto& character = _characters[kvp.second];
			_context.AddContext(ContextSelector::FromRole(role)[0], character);
		}
		_context.SetValue("__num_bots", GetBotCount());

		// Set role aliases
		for (auto& slot : _scenario.GetRoleSlots())
		{
			if (std::ranges::find(Constants::Chat::ReservedCharacterIDs, slot.id) == std::cend(Constants::Chat::ReservedCharacterIDs))
			{
				_context.SetAlias(slot.id, ContextSelector::FromRole(slot.role));
				_context.SetMacro(slot.id, std::format("{{{}:name}}", (fig::string)slot.id));
			}
		}

		// Scenario
		if (auto firstStep = _scenario.GetStory().GetStep(0uz, 0uz)) //! @todo: current step
		{
			fig::string scenario = eval_text((*firstStep).content, _context);
			_context.SetValue("scenario", scenario);
		}
	}

	const fig::string& ChatStaging::GetGrammar() const
	{
		return _promptScaffold.grammar;
	}

	fig::string ChatStaging::GetStoryTitle() const noexcept
	{
		auto& title = _scenario.GetStory().title;
		if (not empty_or_whitespace(title))
			return eval_text(title, _context);
		return "Untitled";
	}
} // namespace
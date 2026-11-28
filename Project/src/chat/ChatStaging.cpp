#include <pch.h>
#include "chat/ChatStaging.h"
#include "chat/PromptBuilder.h"
#include "app/AppState.h"
#include "io/FileUtility.h"
#include "llm/LLMUtility.h"
#include "gui/GUIUtility.h"
#include "gui/Window.h"
#include "text/TextEvaluator.h"

#include <exception>
#include <cassert>

using namespace fig::io;
using namespace fig::data;
using namespace fig::gui;

namespace fig::chat
{
	ChatStaging::ChatStaging(const fig::data::Scenario& scenario, fig::uuid scenarioId, const fig::data::World& world, fig::uuid worldId, const PromptScaffold& scaffold, ChatOptions options) :
		_scenario { _Scenario { .assetId = scenarioId, .instance { scenario } } },
		_world { _World { .assetId = worldId, .instance { world } } },
		_promptScaffold { scaffold },
		_options { options }
	{
	}

	bool ChatStaging::AddCharacter(Role role, const fig::uuid& characterId, const fig::data::Character& character)
	{
		if (characterId.empty())
			return false;
		if (_charactersByRole.contains(role))
			return false; // Role already assigned
		if (_charactersByID.contains(characterId))
			return false; // Character already added

		size_t index = _characters.size();

		_characters.push_back(_Character {
			.assetId = characterId,
			.role = role,
			.instance = character,
		});

		auto& added = _characters.back().instance;
		if (role == Role::User)
			added.chatId = "USR";
		else if (role == Role::Bot1)
			added.chatId = "AI"; //! @id

		_charactersByID[characterId] = index;
		_charactersByRole[role] = index;

		_bDirtyContext = true;
		return true;
	}

	int32_t ChatStaging::GetBotCount() const noexcept
	{
		return static_cast<int32_t>(std::ranges::count_if(_charactersByRole, [](auto& kvp) { return is_bot(kvp.first); }));
	}

	fig::optional_cref<fig::data::Character> ChatStaging::GetCharacterByRole(Role role) const noexcept
	{
		if (auto itFind = _charactersByRole.find(role); itFind != _charactersByRole.cend())
			return make_optional_cref(_characters[itFind->second].instance);
		return fig::nullref;
	}

	bool ChatStaging::HasCharacterInRole(Role role) const noexcept
	{
		return _charactersByRole.contains(role);
	}

	std::optional<fig::uuid> ChatStaging::GetCharacterIdByRole(Role role) const noexcept
	{
		if (auto itFind = _charactersByRole.find(role); itFind != _charactersByRole.cend())
			return find_key(_charactersByID, itFind->second);
		return std::nullopt;
	}

	fig::optional_cref<fig::data::Character> ChatStaging::GetCharacterById(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _charactersByID.find(id); itFind != _charactersByID.cend())
			return make_optional_cref(_characters[itFind->second].instance);
		return fig::nullref;
	}

	fig::optional_cref<fig::data::Character> ChatStaging::GetCharacterByChatId(const fig::string& identifier) const noexcept
	{
		if (identifier.empty() || _characters.empty())
			return fig::nullref;

		if (auto itFind = std::ranges::find_if(_characters, [&identifier](auto& c) { return equals(c.instance.chatId, identifier, true); }); itFind != std::ranges::cend(_characters))
			return make_optional_cref((*itFind).instance);
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
		else if (handle == "char1" or handle == "bot1")
			return Role::Bot1;
		else if (handle == "char2" or handle == "bot2")
			return Role::Bot2;
		else if (handle == "char3" or handle == "bot3")
			return Role::Bot3;
		else if (handle == "char4" or handle == "bot4")
			return Role::Bot4;

		// Role slots
		auto& slots = GetScenario().GetRoleSlots();
		if (auto itSlot = std::ranges::find(slots, handle, [](auto&& s) { return s.customId; }); itSlot != std::cend(slots))
			return (*itSlot).role;
		return Role::Undefined;
	}

	fig::optional_cref<fig::data::Character> ChatStaging::GetCharacterByRole(fig::handle handle) const noexcept
	{
		return GetCharacterByRole(GetRoleFromHandle(handle));
	}

	fig::optional_cref<fig::data::Character> ChatStaging::GetCharacterByName(const fig::string& name) const noexcept
	{
		if (name.empty() || _characters.empty())
			return std::nullopt;

		if (auto itFind = std::ranges::find_if(_characters, [&name](auto& c) { return equals(c.instance.GetSpokenName(), name, true); }); itFind != std::ranges::cend(_characters))
			return make_optional_cref((*itFind).instance);
		return fig::nullref;
	}

	std::vector<fig::data::Character> ChatStaging::GetCharacters() const noexcept
	{ 
		return _characters
			| std::views::filter([](auto&& c) { return is_bot(c.role); })
			| std::views::transform([](auto&& c) { return c.instance; })
			| std::ranges::to<std::vector>();
	}

	Role ChatStaging::GetRoleOf(const fig::string& characterId) const
	{
		if (characterId.empty() || _characters.empty())
			return Role::Undefined;

		if (auto itFind = std::ranges::find_if(_charactersByRole, [this, characterId](const auto& kvp) { 
			return equals(_characters[kvp.second].instance.chatId, characterId, true) 
				or equals(_characters[kvp.second].instance.GetSpokenName(), characterId, true);
			}); itFind != std::ranges::cend(_charactersByRole))
		{
			return itFind->first;
		}
		return Role::Undefined;
	}

	fig::gui::color_triple ChatStaging::GetColorsOf(Role role) const
	{
		if (auto try_character = GetCharacterByRole(role))
		{
			auto& character = *try_character;
			if (character.bgColor.IsDefined() && character.borderColor.IsDefined())
			{
				return fig::gui::color_triple {
					.background = custom_color(character.bgColor),
					.foreground = Color::MessageForegroundDefault,
					.border = custom_color(character.borderColor),
				};
			}
		}

		if (is_bot(role))
		{
			return fig::gui::color_triple {
				.background = BotMessageBackgrounds[get_bot_index(role) % 8],
				.foreground = BotMessageForegrounds[get_bot_index(role) % 8],
				.border = BotMessageBorders[get_bot_index(role) % 8],
			};
		}
		else if (role == Role::User)
		{
			return fig::gui::color_triple {
				.background = DefaultUserMessageBackground,
				.foreground = DefaultUserMessageForeground,
				.border = DefaultUserMessageBorder,
			};
		}
		else if (role == Role::System)
		{
			return fig::gui::color_triple {
				.background = Color::MessageBackgroundNavy,
				.foreground = Color::MessageForegroundNavy,
				.border = Color::MessageBorderNavy,
			};
		}
		else if (role == Role::Narrator)
		{
			return fig::gui::color_triple {
				.background = Color::MessageBackgroundGray,
				.foreground = Color::MessageForegroundGray,
				.border = Color::MessageBorderGray,
			};
		}
		else
		{
			return fig::gui::color_triple {
				.background = Color::MessageBackgroundDefault,
				.foreground = Color::MessageForegroundDefault,
				.border = Color::MessageBorderDefault,
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
			
			// Compile persona
			fig::string persona; //! @temp: For now. Attributes should not be one contiguous block
			for (auto& attribute : character.GetAttributes())
			{
				if (attribute.id == Constants::CharacterAttributes::Persona)
				{
					// (Imported) persona as is
					persona += std::format("{}\n\n", trim(attribute.value));
					continue;
				}

				switch (attribute.type)
				{
				case CharacterAttribute::ValueType::LongText:
					persona += std::format("{0}:\n{1}\n\n", attribute.name, trim(attribute.value));
					break;
				default:
					persona += std::format("{0}: {1}\n", attribute.name, trim(attribute.value));
					break;
				}
			}

			// Brief
			if (not empty_or_whitespace(character.brief))
				persona += std::format("\n{}\n", trim(character.brief));

			// Traits
			if (character.GetTraits().size() > 0uz)
			{
				persona += std::format("{}'s traits:\n", character.GetSpokenName());

				for (auto& trait : character.GetTraits())
					persona += std::format("- [{0}] {1}\n", trait.name, trim(trait.text));
			}

			return eval_text(rtrim(persona), GetCharacterContext(role));
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

	Context ChatStaging::GetCharacterContext(Role role) noexcept
	{
		if (auto try_character = GetCharacterByRole(role))
		{
			auto ctx = (*try_character).GetContext();
			ctx.SetMacroProvider(Global::GetMacroProvider());
			ctx.SetPrimarySelector({});
			ctx.SetAlias("current", ContextSelector {});
			return ctx;
		}
		return {};
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

		_context.SetValue("__num_bots", GetBotCount());
		_context.SetAlias("current", ContextSelector::FromRole(Role::Bot1));
		_context.SetPrimarySelector(ContextSelector::FromRole(Role::Bot1));

		// Set role aliases
		for (auto& slot : GetScenario().GetRoleSlots())
		{
			if (not (slot.customId.empty() or is_reserved(slot.customId)))
			{
				_context.SetAlias(slot.customId, ContextSelector::FromRole(slot.role));
				_context.SetMacro(slot.customId, std::format("{{{0}:name}}", (fig::string)slot.customId));
			}
		}

		// Characters
		for (auto& kvp : _charactersByRole)
		{
			auto role = kvp.first;
			auto& character = _characters[kvp.second].instance;
			auto& characterCtx = _context.AddContext(ContextSelector::FromRole(role)[0], character);
		}

		// Roles
		for (auto& role : CharacterRoles)
		{
			if (auto roleSlot = GetScenario().GetRoleSlot(role))
			{
				if (auto characterCtx = _context.TryGetContext(ContextSelector::FromRole(role)[0]))
				{
					if (not empty_or_whitespace((*roleSlot).label))
						(*characterCtx).SetValue("role_name", eval_text((*roleSlot).label, _context));
					if (not empty_or_whitespace((*roleSlot).brief))
						(*characterCtx).SetValue("role_brief", eval_text((*roleSlot).brief, _context));
				}
			}
		}

		// Participants
		fig::string participants;
		for (auto& role : CharacterRoles)
		{
			if (auto character = GetCharacterByRole(role))
			{
				if (auto characterCtx = _context.TryGetContext(ContextSelector::FromRole(role)[0]))
				{
					fig::string text = eval_text("{name}{role_name? ({role_name})}{role_brief?, {role_brief}{.}}", *characterCtx);
					if (not empty_or_whitespace(text))
						participants.append(std::format("- {}\n", text));
				}
			}
		}

		if (not empty_or_whitespace(participants))
			_context.SetValue("__cast", participants);

		// Scenario
		auto& scenario = GetScenario();
		if (auto intro = scenario.GetFirstScene())
		{
			fig::string scenario = eval_text((*intro).content, _context);
			_context.SetValue("__scene", scenario);
		}

		// Genre(s)
		fig::string genres = eval_text(encode_csv(scenario._genres), _context);
		_context.SetValue("__genres", genres);

		// Rules
		fig::string rules;
		int32_t ruleNumber = 1;
		for (auto& rule : GetWorld().GetRules())
			rules.append(eval_text(std::format("{}. {}{{.}}\n", ruleNumber++, process_text(rule)), _context));
		for (auto& rule : GetScenario().GetRules())
			rules.append(eval_text(std::format("{}. {}{{.}}\n", ruleNumber++, process_text(rule)), _context));
		for (auto& [role, idx] : _charactersByRole)
		{
			for (auto& rule : _characters[idx].instance.GetRules())
				rules.append(eval_text(std::format("{}. {}{{.}}\n", ruleNumber++, process_text(rule)), GetCharacterContext(role)));
		}
		if (non_empty(rules))
			_context.SetValue("rules", rules);
	}

	const fig::string& ChatStaging::GetGrammar() const
	{
		return _promptScaffold.grammar;
	}

	fig::string ChatStaging::GetStoryTitle() const noexcept
	{
		if (not empty_or_whitespace(_scenario.instance.name))
			return _scenario.instance.name;

		fig::string title;
		if (auto characters = GetCharacters(); not characters.empty())
		{
			for (size_t i = 0uz; i < characters.size(); ++i)
			{
				if (i > 0uz)
				{
					title += ", ";
					if (i + 1uz == characters.size())
						title += "and ";
				}
				title += characters[i].GetSpokenName();
			}
			return title;
		}
		return "Untitled";
	}

	std::vector<fig::uuid> ChatStaging::GetCharacterIds() const noexcept
	{
		std::vector<_Character> characters = _characters; // copy
		std::ranges::sort(characters, std::ranges::less(), [](auto&& c) { return c.role; });
		return characters
			| std::views::filter([](auto&& c) { return is_bot(c.role); })
			| std::views::transform([](auto&& c) { return c.assetId; })
			| std::ranges::to<std::vector>();
	}

	std::optional<fig::uuid> ChatStaging::GetUserId() const noexcept
	{
		std::vector<_Character> characters = _characters; // copy
		if (auto itFind = std::ranges::find_if(_characters, [](auto&& c) { return is_user(c.role); }); itFind != std::ranges::cend(_characters))
			return (*itFind).assetId;
		return std::nullopt;
	}
} // namespace
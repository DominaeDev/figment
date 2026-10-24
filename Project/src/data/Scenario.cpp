#include <pch.h>
#include "data/Scenario.h"
#include "text/ConditionParser.h"
#include "text/TextEvaluator.h"
#include "io/Xml.h"
#include <cassert>

using namespace fig::io;
using namespace fig::chat;

namespace fig::data
{
	bool Scenario::OnLoadFromXml(XmlReaderElement xml)
	{
		size_t bot_index = 0uz;
		for (auto& slot : roles)
		{
			if (slot.flags.IsSet(RoleSlot::Flag::User))
				slot.role = Role::User;
			else
				slot.role = bot_from_index(bot_index++);
		}

		return true;
	}

	void Scenario::SetRules(const std::vector<ScenarioRule>& rules) noexcept
	{
		_rules = rules;
	}

	void Scenario::AppendRules(const std::vector<ScenarioRule>& rules) noexcept
	{
		_rules.append_range(rules);
	}

	void Scenario::ClearRules() noexcept
	{
		_rules.clear();
	}

	bool Scenario::OnValidate() const noexcept
	{
		if constexpr (Enabled)
		{
			if (roles.empty())
				return false;

			// Ensure the scenario can assign roles to at least one bot and one user
			int32_t idx_bot { -1 };
			int32_t idx_user { -1 };
			for (size_t idx = 0; idx < roles.size(); ++idx)
			{
				if (idx_user < 0 and roles[idx].flags.IsSet(RoleSlot::Flag::User))
				{
					idx_user = static_cast<int32_t>(idx);
					continue;
				}
				if (idx_bot < 0 and idx != idx_user)
				{
					idx_bot = static_cast<int32_t>(idx);
					continue;
				}
			}

			if (idx_bot == -1 or idx_user == -1)
				return false; // No exclusive roles
			return true;
		}
		else
		{
			return true;
		}
	}

	fig::optional_cref<Scenario::Scene> Scenario::GetFirstScene() const noexcept
	{
		if (auto it = std::ranges::find_if(scenes, [](auto&& s) { return s.bEntryPoint; }); it != std::ranges::cend(scenes))
			return *it;
		if (not scenes.empty())
			return scenes.front();
		return std::nullopt;
	}

	fig::string Scenario::GetTitle(const Context& context) const noexcept
	{
		if (not empty_or_whitespace(title))
			return eval_text(title, context);
		return "Untitled";
	}

	Scenario::Scene& Scenario::CreateScene() noexcept
	{
		scenes.emplace_back(Scene {
			.id = _CreateUUID(),
			.bEntryPoint = scenes.empty(),
		});
		return scenes.back();
	}
}
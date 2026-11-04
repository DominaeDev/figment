#include <pch.h>
#include "data/World.h"
#include "text/ConditionParser.h"
#include "text/TextEvaluator.h"
#include "io/Xml.h"
#include <cassert>

using namespace fig::io;
using namespace fig::chat;

namespace fig::data
{
	void World::SetRules(const std::vector<WorldRule>& rules) noexcept
	{
		_rules = rules;
	}

	void World::AppendRules(const std::vector<WorldRule>& rules) noexcept
	{
		_rules.append_range(rules);
	}

	void World::ClearRules() noexcept
	{
		_rules.clear();
	}

	void World::SetTags(const fig::string_list& tags) noexcept
	{
		_tags = tags;
	}

	void World::AppendTags(const fig::string_list& tags)
	{
		_tags.append_range(tags);
	}
}
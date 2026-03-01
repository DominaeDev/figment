#include <pch.h>
#include "llm/LLMStateVariables.h"
#include "util/StringUtility.h"
#include <format>

using namespace fig::llm;
using namespace fig::util;

bool LLMStateVariables::SetValue(fig::string name, fig::string value)
{
	if (!value.empty())
	{
		auto itFind = std::find_if(_variables.begin(), _variables.end(), [&name](auto kvp) {
			return equals(kvp.first, name, true);
		});
		
		if (itFind != _variables.end())
		{
			if (itFind->second != value)
			{
				itFind->second = value;
				return true;
			}
			return false; // Unchanged
		}
		else
		{
			_variables[name] = value;
			return true;
		}
	}

	return _variables.erase(name) > 0;
}

bool LLMStateVariables::HasValue(fig::string name) const
{
	auto itFind = std::find_if(_variables.begin(), _variables.end(), [&name](auto kvp) {
		return equals(kvp.first, name, true);
	});
		
	return itFind != _variables.end();
}

fig::string LLMStateVariables::GetList() const
{
	fig::string result;
	for (auto kvp : _variables)
		result.append(std::format("{} = {};\n", kvp.first, kvp.second));
	return result;
}

fig::string LLMStateVariables::GetGrammarPattern() const
{
	if (_variables.empty())
		return "[]";

	fig::string result;
	int i = 0;
	for (auto it = _variables.begin(); it != _variables.end(); ++it, ++i)
	{
		if (i > 0)
			result.append(" | ");
		result.append(std::format("\"{}\"", (*it).first));
	}
	return result;
}

void LLMStateVariables::UpdateValues(fig::string stateReport, std::map<fig::string, fig::string>& updatedVariables)
{
	replace_all_inplace(stateReport, "<change>", "");
	replace_all_inplace(stateReport, "</change>", ";");
	auto rows = split(stateReport, ';', true);

	for (auto& row : rows)
	{
		size_t pos_eq = row.find("=");
		if (pos_eq == fig::npos)
			continue;

		fig::string lhs = trim(row.substr(0, pos_eq));
		fig::string rhs = trim(row.substr(pos_eq + 1));
		if (SetValue(lhs, rhs))
			updatedVariables[lhs] = rhs;
	}
}
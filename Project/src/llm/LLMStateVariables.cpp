#include "llm/LLMStateVariables.h"
#include "util/StringUtility.h"
#include <format>

bool LLMStateVariables::SetValue(string name, string value)
{
	if (!value.empty())
	{
		auto itFind = std::find_if(_variables.begin(), _variables.end(), [&name](auto kvp) {
			return string_util::equals(kvp.first, name, true);
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

bool LLMStateVariables::HasValue(string name) const
{
	auto itFind = std::find_if(_variables.begin(), _variables.end(), [&name](auto kvp) {
		return string_util::equals(kvp.first, name, true);
	});
		
	return itFind != _variables.end();
}

string LLMStateVariables::GetList() const
{
	string result;
	for (auto kvp : _variables)
		result.append(std::format("{} = {};\n", kvp.first, kvp.second));
	return result;
}

string LLMStateVariables::GetGrammarPattern() const
{
	if (_variables.empty())
		return "[]";

	string result;
	int i = 0;
	for (auto it = _variables.begin(); it != _variables.end(); ++it, ++i)
	{
		if (i > 0)
			result.append(" | ");
		result.append(std::format("\"{}\"", (*it).first));
	}
	return result;
}

void LLMStateVariables::UpdateValues(string stateReport, std::map<string, string>& updatedVariables)
{
	string_util::replace_all(stateReport, "<change>", "");
	string_util::replace_all(stateReport, "</change>", ";");
	auto rows = string_util::split(stateReport, ';', true);

	for (auto row : rows)
	{
		size_t pos_eq = row.find("=");
		if (pos_eq == string::npos)
			continue;

		string lhs = string_util::trim(row.substr(0, pos_eq));
		string rhs = string_util::trim(row.substr(pos_eq + 1));
		if (SetValue(lhs, rhs))
			updatedVariables[lhs] = rhs;
	}
}
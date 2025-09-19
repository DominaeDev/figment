#pragma once

#include "Types.h"

class LLMStateVariables
{
public:
	bool SetValue(string name, string value);
	bool HasValue(string name) const;
	bool IsEmpty() const { return _variables.empty(); }
	void UpdateValues(string stateReport, std::map<string, string>& updatedVariables);

	string GetList() const;
	string GetGrammarPattern() const;
	const std::map<string, string>& GetVariables() const { return _variables; }

private:
	std::map<string, string> _variables;
};
#pragma once

#include "Types.h"

class LLMState
{
public:
	void SetValue(string name, string value);
	bool IsEmpty() const { return _variables.empty(); }
	void UpdateValues(string stateReport);

	string GetList() const;
	string GetGrammarPattern() const;
	const std::map<string, string>& GetVariables() const { return _variables; }

private:
	std::map<string, string> _variables;
};
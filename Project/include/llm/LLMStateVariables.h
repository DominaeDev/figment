#pragma once

#include "Figment.h"
#include <map>

namespace fig::llm
{
	class LLMStateVariables
	{
	public:
		bool SetValue(fig::string name, fig::string value);
		bool HasValue(fig::string name) const;
		bool IsEmpty() const { return _variables.empty(); }
		void UpdateValues(fig::string stateReport, std::map<fig::string, fig::string>& updatedVariables);

		fig::string GetList() const;
		fig::string GetGrammarPattern() const;
		const std::map<fig::string, fig::string>& GetVariables() const { return _variables; }

	private:
		std::map<fig::string, fig::string> _variables;
	};
}
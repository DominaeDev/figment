#include <pch.h>
#include "text/Condition.h"
#include "text/ConditionParser.h"

namespace fig
{
	Condition::Condition(const fig::string& expression)
	{
		if (auto try_parse = ConditionParser::Parse(expression))
		{
			_pCondition = std::move(try_parse.value());
			_error = {};
		}
		else
		{
			_pCondition.reset();
			_error = try_parse.error();
		}
	}

	bool Condition::Evaluate(const Contextual& context) const
	{
		return _pCondition && _pCondition->Evaluate(context);
	}
}
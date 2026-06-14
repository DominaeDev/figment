#include <pch.h>
#include "text/Condition.h"
#include "text/ConditionParser.h"

namespace fig
{
	const Condition Condition::Always { "always" };

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

	Condition::Condition(const Condition& other)
	{
		operator=(other);
	}

	Condition& Condition::operator= (const Condition& other) noexcept
	{
		if (other._pCondition)
			_pCondition = other._pCondition->Clone();
		else
			_pCondition.reset();
		return *this;
	}

	bool Condition::Evaluate(const Context& context) const
	{
		return !_pCondition or _pCondition->Evaluate(context);
	}

	EvaluationResult Condition::Evaluate(const fig::string& expression, const Context& context)
	{
		Condition condition(expression);
		if (condition._error != ConditionParseError::NoError)
			return EvaluationResult::Error(condition._error);
		return condition.Evaluate(context);
	}

	Condition::operator fig::string() const noexcept
	{
		return _pCondition ? (fig::string)(*_pCondition) : "";
	}

	bool Condition::IsOk() const noexcept
	{
		return _error == ConditionParseError::NoError;
	}
}
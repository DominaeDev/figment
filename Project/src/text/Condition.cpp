#include <pch.h>
#include "text/Condition.h"
#include "text/Context.h"
#include "text/ConditionParser.h"

using namespace fig::data;

namespace fig
{
	const Condition Condition::Always { "always" };

	bool Condition::LoadFromXml(XmlReaderElement xml) noexcept
	{
		if (auto try_value = xml.TryGetValue<fig::string>())
		{
			if (auto try_parse = ConditionParser::Parse(try_value.value()))
			{
				_pCondition = std::move(try_parse.value());
				_error = {};
				return true;
			}
			else
				_error = try_parse.error();
		}
		_pCondition.reset();
		return false;
	}

	void Condition::SaveToXml(XmlWriterElement xml) const noexcept
	{
		xml.SetValue((fig::string)(*this));
	}

	Condition::Condition(const fig::string& expression, bool defaultToAlways)
	{
		if (empty_or_whitespace(expression) and defaultToAlways)
		{
			_pCondition = std::make_unique<AlwaysCondition>();
			_error = {};
		}
		else if (auto try_parse = ConditionParser::Parse(expression))
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

	bool Condition::Evaluate(const Context& context, size_t cookie) const
	{
		if (cookie >= 100)
		{
			LogLn("Error: Condition evaluation stack overflow!");
			return false;
		}

		EvaluationArgs eval { context, cookie };
		return !_pCondition or _pCondition->Evaluate(eval);
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
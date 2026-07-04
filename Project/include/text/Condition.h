#pragma once

#include "io/Xml.h"
#include "text/ConditionNode.h"

namespace fig
{
	class Context;
	enum class ConditionParseError;

	struct EvaluationResult
	{
		EvaluationResult(bool value) :
			_state(value ? State::True : State::False)
		{
		}

		operator bool() const { return _state == State::True; }
		bool IsError() const { return _state == State::Error; }

		static EvaluationResult Error(ConditionParseError error) { return EvaluationResult(error); }

	private:
		enum class State : uint8_t { False, True, Error };

		explicit EvaluationResult(State state) :
			_state(state)
		{
		}

		explicit EvaluationResult(ConditionParseError error) :
			_state(State::Error),
			_error(error)
		{
		}

		State _state;
		ConditionParseError _error {};
	};

	class Condition
	{
	public:
		Condition() = default;
		Condition(const fig::string& expression, bool defaultToAlways = false);
		Condition(const Condition& other);
		Condition(Condition&& other) = default;
		Condition& operator= (const Condition& other) noexcept;
		Condition& operator= (Condition&& other) noexcept = default;

		bool Evaluate(const Context& context, size_t cookie = 0uz) const;
		static EvaluationResult Evaluate(const fig::string& expression, const Context& context);

		explicit operator fig::string() const noexcept;
		bool IsOk() const noexcept;

		static const Condition Always;
		
		bool LoadFromXml(fig::data::XmlReaderElement xml) noexcept;
		void SaveToXml(fig::data::XmlWriterElement xml) const noexcept;

	private:
		ConditionPtr _pCondition;
		ConditionParseError _error {};
	};

	struct Validation
	{
		Condition condition;
		fig::string errorMessage;

		static auto XmlFields() noexcept
		{
			using namespace fig::data;

			return Fields(
				Element { "Condition",	&Validation::condition }
					.Default(Condition::Always),
				Attribute { "error",		&Validation::errorMessage }
			);
		}
	};

}

#ifndef CONDITION_H__
#define CONDITION_H__
#pragma once

#include "Contextual.h"
#include "ConditionNode.h"

namespace fig
{
	class Condition
	{
	public:
		Condition() = default;
		Condition(const fig::string& expression);
		Condition(const Condition& other) = delete;
		Condition(Condition&& other) = default;
		Condition& operator= (const Condition& other) noexcept = delete;
		Condition& operator= (Condition&& other) noexcept = default;

		bool Evaluate(const Contextual& context) const;

	private:
		ConditionPtr _pCondition;
		ConditionParseError _error {};
	};
}

#endif
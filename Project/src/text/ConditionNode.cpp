#include <pch.h>
#include "text/ConditionNode.h"
#include "text/Context.h"

namespace fig
{
	AndCondition::AndCondition(std::vector<ConditionPtr> children) :
		_children(std::move(children))
	{
	}

	bool AndCondition::Evaluate(const Context& context) const
	{
		for (const auto& child : _children)
		{
			if (!child->Evaluate(context))
				return false;
		}
		return !_children.empty();
	}

	OrCondition::OrCondition(std::vector<ConditionPtr> children) :
		_children(std::move(children))
	{
	}

	bool OrCondition::Evaluate(const Context& context) const
	{
		for (const auto& child : _children)
		{
			if (child->Evaluate(context))
				return true;
		}
		return false;
	}

	NotCondition::NotCondition(ConditionPtr child) :
		_child(std::move(child))
	{
	}

	bool NotCondition::Evaluate(const Context& context) const
	{
		return _child && !_child->Evaluate(context);
	}

	ComparisonCondition::ComparisonCondition(const fig::string& lhs, CompareOperator op, RhsValue rhs) :
		_operator(op),
		_rhs(std::move(rhs))
	{
		_location = { lhs };
	}

	bool ComparisonCondition::Evaluate(const Context& context) const
	{
		if (auto pNumber = std::get_if<float>(&_rhs))
		{
			if (auto try_value = context.TryGetValue<float>(_location))
			{
				auto value = try_value.value();
				switch (_operator)
				{
					case CompareOperator::Equal:
						return flt_eq(value, *pNumber);
					case CompareOperator::NotEqual:
						return !flt_eq(value, *pNumber);
					case CompareOperator::LessThan:
						return value < *pNumber;
					case CompareOperator::LessOrEqual:
						return value <= *pNumber;
					case CompareOperator::GreaterThan:
						return value > *pNumber;
					case CompareOperator::GreaterOrEqual:
						return value >= *pNumber;
				}
			}
		}
		else if (auto pText = std::get_if<fig::string>(&_rhs))
		{
			if (auto try_value = context.TryGetValue<fig::string>(_location))
			{
				auto& value = try_value.value();
				switch (_operator)
				{
					case CompareOperator::Equal:
						return equals(value, *pText, true);
					case CompareOperator::NotEqual:
						return !equals(value, *pText, true);
					case CompareOperator::LessThan:
						return value < *pText;
					case CompareOperator::LessOrEqual:
						return value <= *pText;
					case CompareOperator::GreaterThan:
						return value > *pText;
					case CompareOperator::GreaterOrEqual:
						return value >= *pText;
				}
			}
		}
		return false;
	}

	FlagCondition::FlagCondition(const fig::string& location)
	{
		_location = { location };
	}

	bool FlagCondition::Evaluate(const Context& context) const
	{
		if (_location.key.empty())
			return false;

		return context[_location];
	}

}
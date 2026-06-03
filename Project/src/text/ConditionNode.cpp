#include <pch.h>
#include "text/ConditionNode.h"
#include "text/Contextual.h"

namespace fig
{
	AndCondition::AndCondition(std::vector<ConditionPtr> children) :
		_children(std::move(children))
	{
	}

	bool AndCondition::Evaluate(const Contextual& context) const
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

	bool OrCondition::Evaluate(const Contextual& context) const
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

	bool NotCondition::Evaluate(const Contextual& context) const
	{
		return _child && !_child->Evaluate(context);
	}

	ComparisonCondition::ComparisonCondition(const fig::string& lhs, CompareOperator op, RhsValue rhs) :
		_operator(op),
		_rhs(std::move(rhs))
	{
		if (size_t pos_selector = lhs.find(':'); pos_selector != npos)
		{
			_selector = Selector { trim(lhs.substr(0, pos_selector)) };
			_key = trim(lhs.substr(pos_selector + 1));
		}
		else
			_key = trim(lhs);
	}

	bool ComparisonCondition::Evaluate(const Contextual& context) const
	{
		if (auto try_ctx = context.TryGetContext(_selector))
		{
			auto& ctx = try_ctx.value().get();
			if (auto pNumber = std::get_if<float>(&_rhs))
			{
				if (auto try_value = ctx.TryGetValue<float>(_key))
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
				if (auto try_value = ctx.TryGetValue<fig::string>(_key))
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
		}
		
		return false;
	}

	FlagCondition::FlagCondition(const fig::string& name)
	{
		if (size_t pos_selector = name.find(':'); pos_selector != npos)
		{
			_selector = Selector { trim(name.substr(0, pos_selector)) };
			_key = trim(name.substr(pos_selector + 1));
		}
		else
			_key = trim(name);
	}

	bool FlagCondition::Evaluate(const Contextual& context) const
	{
		if (_key.empty())
			return false;

		if (auto try_ctx = context.TryGetContext(_selector))
			return try_ctx.value().get()[_key];
		return false;
	}

}
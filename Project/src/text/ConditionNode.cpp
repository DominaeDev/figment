#include <pch.h>
#include "text/ConditionNode.h"
#include "text/Context.h"
#include <cassert>

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

	ConditionPtr AndCondition::Clone() const
	{
		std::vector<ConditionPtr> children;
		for (const auto& child : _children)
			children.push_back(child->Clone());
		return std::make_unique<AndCondition>(std::move(children));
	}

	AndCondition::operator fig::string() const
	{
		if (_children.size() == 1)
			return (fig::string)(*_children[0]);
		else if (_children.size() > 1)
		{
			return std::format("({})",
				_children
				| std::views::transform([](auto&& c) { return (fig::string)(*c); })
				| std::views::join_with(std::string_view { " and " })
				| std::ranges::to<std::string>()
			);
		}
		return "#error";
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

	ConditionPtr OrCondition::Clone() const
	{
		std::vector<ConditionPtr> children;
		for (const auto& child : _children)
			children.push_back(child->Clone());
		return std::make_unique<OrCondition>(std::move(children));
	}

	OrCondition::operator fig::string() const
	{
		if (_children.size() == 1)
			return (fig::string)(*_children[0]);
		else if (_children.size() > 1)
		{
			return _children
				| std::views::transform([](auto&& c) { return (fig::string)(*c); })
				| std::views::join_with(std::string_view { " or " })
				| std::ranges::to<std::string>();
		}
		return "#error";
	}

	NotCondition::NotCondition(ConditionPtr child) :
		_child(std::move(child))
	{
	}

	bool NotCondition::Evaluate(const Context& context) const
	{
		return _child && !_child->Evaluate(context);
	}

	ConditionPtr NotCondition::Clone() const
	{
		return std::make_unique<NotCondition>(_child->Clone());
	}

	NotCondition::operator fig::string() const
	{
		if (_child)
			return std::format("not ({})", (fig::string)(*_child));
		return "#error";
	}

	ComparisonCondition::ComparisonCondition(CompareOperand lhs, CompareOperand rhs, CompareOperator op) :
		_lhs(std::move(lhs)),
		_rhs(std::move(rhs)),
		_operator(op)
	{
	}

	static bool compare_variants(CompareOperand lhs, CompareOperand rhs, CompareOperator op)
	{
		// Float
		if (auto try_lhs_flt = std::get_if<float>(&lhs))
		{
			float lhs_flt { *try_lhs_flt };
			float rhs_flt;
			if (auto try_rhs_flt = std::get_if<float>(&rhs))
				rhs_flt = *try_rhs_flt;
			else if (auto try_rhs_str = std::get_if<fig::string>(&rhs))
			{
				if (auto try_parse = string_to_float(*try_rhs_str))
					rhs_flt = try_parse.value();
				else
					return false; // Error
			}
			else
				return false; // Error

			switch (op)
			{
				case CompareOperator::Equal:
					return flt_eq(lhs_flt, rhs_flt);
				case CompareOperator::NotEqual:
					return !flt_eq(lhs_flt, rhs_flt);
				case CompareOperator::LessThan:
					return lhs_flt < rhs_flt;
				case CompareOperator::LessOrEqual:
					return lhs_flt <= rhs_flt;
				case CompareOperator::GreaterThan:
					return lhs_flt > rhs_flt;
				case CompareOperator::GreaterOrEqual:
					return lhs_flt >= rhs_flt;
			}
			return false; // Error
		}

		// Text
		else if (auto try_lhs_str = std::get_if<fig::string>(&lhs))
		{
			fig::string lhs_str { *try_lhs_str };
			fig::string rhs_str;
			if (auto try_rhs_flt = std::get_if<float>(&rhs))
				rhs_str = float_to_string(*try_rhs_flt);
			else if (auto try_rhs_str = std::get_if<fig::string>(&rhs))
				rhs_str = *try_rhs_str;
			else
				return false; // Error

			switch (op)
			{
				case CompareOperator::Equal:
					return equals(lhs_str, rhs_str, true);
				case CompareOperator::NotEqual:
					return !equals(lhs_str, rhs_str, true);
				case CompareOperator::LessThan:
					return lhs_str < rhs_str;
				case CompareOperator::LessOrEqual:
					return lhs_str <= rhs_str;
				case CompareOperator::GreaterThan:
					return lhs_str > rhs_str;
				case CompareOperator::GreaterOrEqual:
					return lhs_str >= rhs_str;
			}
			return false; // Error
		}

		assert(false && "Invalid operand type");
		return false; // Error
	}

	bool ComparisonCondition::Evaluate(const Context& context) const
	{
		CompareOperand lhs {};
		if (auto lhs_flt = std::get_if<float>(&_lhs))
		{
			lhs = *lhs_flt;
		}
		else if (auto lhs_str = std::get_if<fig::string>(&_lhs))
		{
			if (auto try_value = context.TryGetRaw(ContextLocator { *lhs_str }))
			{
				auto& value = try_value.value();
				if (auto i = std::get_if<int32_t>(&value))
					lhs = static_cast<float>(*i);
				else if (auto f = std::get_if<float>(&value))
					lhs = *f;
				else if (auto s = std::get_if<fig::string>(&value))
					lhs = *s;
			}
			else
				lhs = *lhs_str;
		}

		CompareOperand rhs {};
		if (auto rhs_flt = std::get_if<float>(&_rhs))
		{
			rhs = *rhs_flt;
		}
		else if (auto rhs_str = std::get_if<fig::string>(&_rhs))
		{
			if (auto try_value = context.TryGetRaw(ContextLocator { *rhs_str }))
			{
				auto& value = try_value.value();
				if (auto i = std::get_if<int32_t>(&value))
					rhs = static_cast<float>(*i);
				else if (auto f = std::get_if<float>(&value))
					rhs = *f;
				else if (auto s = std::get_if<fig::string>(&value))
					rhs = *s;
			}
			else
				rhs = *rhs_str;
		}

		return compare_variants(lhs, rhs, _operator);
	}

	ConditionPtr ComparisonCondition::Clone() const
	{
		return std::make_unique<ComparisonCondition>(_lhs, _rhs, _operator);
	}

	ComparisonCondition::operator fig::string() const
	{
		fig::string lhs;
		if (auto lhs_flt = std::get_if<float>(&_lhs))
			lhs = std::format("{:g}", *lhs_flt);
		else if (auto lhs_str = std::get_if<fig::string>(&_lhs))
			lhs = *lhs_str;
		
		fig::string rhs;
		if (auto rhs_flt = std::get_if<float>(&_rhs))
			rhs = std::format("{:g}", *rhs_flt);
		else if (auto rhs_str = std::get_if<fig::string>(&_rhs))
			rhs = *rhs_str;

		fig::string op;
		switch (_operator)
		{
			case CompareOperator::Equal: op = "="; break;
			case CompareOperator::NotEqual: op = "!="; break;
			case CompareOperator::LessThan: op = "<"; break;
			case CompareOperator::LessOrEqual: op = "<="; break;
			case CompareOperator::GreaterThan: op = ">"; break;
			case CompareOperator::GreaterOrEqual: op = ">="; break;
			default: op = "#error"; break;
		}

		return std::format("{} {} {}", lhs, op, rhs);
	}

	FlagCondition::FlagCondition(const fig::string& flag) :
		_flag(flag)
	{
	}

	FlagCondition::FlagCondition(const ContextLocator& flag) :
		_flag(flag)
	{
	}

	bool FlagCondition::Evaluate(const Context& context) const
	{
		if (_flag.key.empty())
			return false;

		return context[_flag];
	}

	ConditionPtr FlagCondition::Clone() const
	{
		return std::make_unique<FlagCondition>(_flag);
	}

	FlagCondition::operator fig::string() const
	{
		return (fig::string)_flag;
	}

	bool AlwaysCondition::Evaluate(const Context& context) const
	{
		return true;
	}

	ConditionPtr AlwaysCondition::Clone() const
	{
		return std::make_unique<AlwaysCondition>();
	}

	AlwaysCondition::operator fig::string() const
	{
		return "always";
	}

	bool NeverCondition::Evaluate(const Context& context) const
	{
		return false;
	}

	ConditionPtr NeverCondition::Clone() const
	{
		return std::make_unique<NeverCondition>();
	}

	NeverCondition::operator fig::string() const
	{
		return "never";
	}

}
#include <pch.h>
#include "text/ConditionNode.h"
#include "text/Context.h"
#include <cassert>
#include <math.hpp>

namespace fig
{
	AndCondition::AndCondition(std::vector<ConditionPtr> children) :
		_children(std::move(children))
	{
	}

	bool AndCondition::Evaluate(const EvaluationArgs& eval) const
	{
		for (const auto& child : _children)
		{
			if (!child->Evaluate(eval))
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

	bool OrCondition::Evaluate(const EvaluationArgs& eval) const
	{
		for (const auto& child : _children)
		{
			if (child->Evaluate(eval))
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

	bool NotCondition::Evaluate(const EvaluationArgs& eval) const
	{
		return _child && !_child->Evaluate(eval);
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

	using CompareValue = std::variant<fig::fixed, fig::string>;

	static bool compare_variants(CompareValue lhs, CompareValue rhs, CompareOperator op)
	{
		// Number
		if (auto try_lhs_flt = std::get_if<fig::fixed>(&lhs))
		{
			fig::fixed lhs_flt { *try_lhs_flt };
			fig::fixed rhs_flt;
			if (auto try_rhs_flt = std::get_if<fig::fixed>(&rhs))
				rhs_flt = *try_rhs_flt;
			else if (auto try_rhs_str = std::get_if<fig::string>(&rhs))
			{
				if (auto try_parse = string_to_float(*try_rhs_str))
					rhs_flt = toFixed(*try_parse);
				else
					return false; // Error
			}
			else
				return false; // Error

			switch (op)
			{
				case CompareOperator::Equal:
				case CompareOperator::EqualStrict:
					return lhs_flt == rhs_flt;
				case CompareOperator::EqualApprox:
					return rint(lhs_flt) == rint(rhs_flt);
				case CompareOperator::NotEqual:
				case CompareOperator::NotEqualStrict:
					return lhs_flt != rhs_flt;
				case CompareOperator::NotEqualApprox:
					return rint(lhs_flt) != rint(rhs_flt);
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
			if (auto try_rhs_flt = std::get_if<fig::fixed>(&rhs))
				rhs_str = fixed_to_string(*try_rhs_flt);
			else if (auto try_rhs_str = std::get_if<fig::string>(&rhs))
				rhs_str = *try_rhs_str;
			else
				return false; // Error

			switch (op)
			{
				case CompareOperator::Equal:
				case CompareOperator::EqualApprox:
					return equals(lhs_str, rhs_str, true);
				case CompareOperator::EqualStrict:
					return equals(lhs_str, rhs_str, false);
				case CompareOperator::NotEqual:
				case CompareOperator::NotEqualApprox:
					return !equals(lhs_str, rhs_str, true);
				case CompareOperator::NotEqualStrict:
					return !equals(lhs_str, rhs_str, false);
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

	bool ComparisonCondition::Evaluate(const EvaluationArgs& eval) const
	{
		static std::mt19937_64 rng { std::random_device{}() };

		CompareValue lhs {};
		if (auto lhs_flt = std::get_if<fig::fixed>(&_lhs))
		{
			lhs = *lhs_flt;
		}
		else if (auto lhs_str = std::get_if<fig::string>(&_lhs))
		{
			lhs = *lhs_str;
		}
		else if (auto lhs_loc = std::get_if<ContextLocator>(&_lhs))
		{
			if (auto try_value = eval.context.TryGetRaw(*lhs_loc))
			{
				auto& value = try_value.value();
				if (auto f = std::get_if<fig::fixed>(&value))
					lhs = *f;
				else if (auto s = std::get_if<fig::string>(&value))
					lhs = *s;
			}
			else
				lhs = fig::string { *lhs_loc };
		}
		else if (auto lhs_dice = std::get_if<Fraction>(&_lhs))
		{
			std::uniform_int_distribution<int32_t> dist(1, (*lhs_dice).denominator);
			int32_t sum = 0;
			for (int32_t n = 0; n < (*lhs_dice).numerator; ++n)
				sum += dist(rng);
			lhs = toFixed(sum);
		}

		CompareValue rhs {};
		if (auto rhs_flt = std::get_if<fig::fixed>(&_rhs))
		{
			rhs = *rhs_flt;
		}
		else if (auto rhs_flt = std::get_if<fig::string>(&_rhs))
		{
			rhs = *rhs_flt;
		}
		else if (auto rhs_loc = std::get_if<ContextLocator>(&_rhs))
		{
			if (auto try_value = eval.context.TryGetRaw(*rhs_loc))
			{
				auto& value = try_value.value();
				if (auto f = std::get_if<fig::fixed>(&value))
					rhs = *f;
				else if (auto s = std::get_if<fig::string>(&value))
					rhs = *s;
			}
			else
				rhs = fig::string { *rhs_loc };
		}
		else if (auto rhs_dice = std::get_if<Fraction>(&_lhs))
		{
			std::uniform_int_distribution<int32_t> dist(1, (*rhs_dice).denominator);
			int32_t sum = 0;
			for (int32_t n = 0; n < (*rhs_dice).numerator; ++n)
				sum += dist(rng);
			lhs = toFixed(sum);
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
		if (auto lhs_flt = std::get_if<fig::fixed>(&_lhs))
			lhs = fixed_to_string(*lhs_flt);
		else if (auto lhs_str = std::get_if<fig::string>(&_lhs))
			lhs = *lhs_str;
		else if (auto lhs_loc = std::get_if<ContextLocator>(&_lhs))
			lhs = static_cast<fig::string>(*lhs_loc);
		
		fig::string rhs;
		if (auto rhs_flt = std::get_if<fig::fixed>(&_rhs))
			rhs = fixed_to_string(*rhs_flt);
		else if (auto rhs_str = std::get_if<fig::string>(&_rhs))
			rhs = *rhs_str;
		else if (auto rhs_loc = std::get_if<ContextLocator>(&_rhs))
			rhs = fig::string { *rhs_loc };

		fig::string op;
		switch (_operator)
		{
			case CompareOperator::Equal: op = "="; break;
			case CompareOperator::EqualStrict: op = "=="; break;
			case CompareOperator::EqualApprox: op = "~="; break;
			case CompareOperator::NotEqual: op = "!="; break;
			case CompareOperator::NotEqualStrict: op = "!=="; break;
			case CompareOperator::NotEqualApprox: op = "!~="; break;
			case CompareOperator::LessThan: op = "<"; break;
			case CompareOperator::LessOrEqual: op = "<="; break;
			case CompareOperator::GreaterThan: op = ">"; break;
			case CompareOperator::GreaterOrEqual: op = ">="; break;
			default: 
				assert(false && "Invalid operator");
				op = "??"; 
				break;
		}

		return std::format("{} {} {}", lhs, op, rhs);
	}

	FlagCondition::FlagCondition(const ContextLocator& flag) :
		_flag(flag)
	{
	}

	bool FlagCondition::Evaluate(const EvaluationArgs& eval) const
	{
		if (!_flag)
			return false;

		if (!_flag.selector and eval.context[_flag.key])
			return true;

		if (auto try_ctx = eval.context.TryGetContext(_flag.selector))
		{
			auto& ctx = *try_ctx;

			// Alias?
			if (auto try_cond = eval.context.TryGetCondition(_flag.key))
				return (*try_cond).Evaluate(ctx, eval.cookie + 1);
			return ctx[_flag.key];
		}

		return false; // Invalid selector
	}

	ConditionPtr FlagCondition::Clone() const
	{
		return std::make_unique<FlagCondition>(_flag);
	}

	FlagCondition::operator fig::string() const
	{
		return (fig::string)_flag;
	}

	RandomCondition::RandomCondition(int32_t num, int32_t denom) :
		_num(num),
		_denom(denom)
	{
	}
	
	bool RandomCondition::Evaluate(const EvaluationArgs& eval) const
	{
		if (_num <= 0 or _denom <= 0)
			return false;

		static std::mt19937_64 rng { std::random_device{}() };
		std::uniform_int_distribution<int32_t> dist(1, _denom);
		auto roll = dist(rng);
		return roll <= _num;
	}

	ConditionPtr RandomCondition::Clone() const
	{
		return std::make_unique<RandomCondition>(_num, _denom);
	}

	RandomCondition::operator fig::string() const
	{
		return std::format("{}:{}", _num, _denom);
	}

	bool AlwaysCondition::Evaluate(const EvaluationArgs& eval) const
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

	bool NeverCondition::Evaluate(const EvaluationArgs& eval) const
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
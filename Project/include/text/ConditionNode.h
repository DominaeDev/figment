#pragma once

#include "Figment.h"
#include "text/ContextLocator.h"

namespace fig
{
    class Context;
    struct EvaluationArgs
    {
        const Context& context;
        size_t cookie = 0uz;
    };

    class ConditionNode
    {
    public:
        virtual ~ConditionNode() = default;
        virtual bool Evaluate(const EvaluationArgs& eval) const = 0;
        virtual std::unique_ptr<ConditionNode> Clone() const = 0;

        virtual explicit operator fig::string() const = 0;
    };

    using ConditionPtr = std::unique_ptr<ConditionNode>;

    class AndCondition : public ConditionNode
    {
    public:
        explicit AndCondition(std::vector<ConditionPtr> children);
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;

    private:
        std::vector<ConditionPtr> _children;
    };

    class OrCondition : public ConditionNode
    {
    public:
        explicit OrCondition(std::vector<ConditionPtr> children);
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    private:
        std::vector<ConditionPtr> _children;
    };

    class NotCondition : public ConditionNode
    {
    public:
        explicit NotCondition(ConditionPtr child);
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    private:
        ConditionPtr _child;
    };

    enum class CompareOperator
    {
        Equal,
        EqualStrict,
        EqualApprox,
        NotEqual,
        NotEqualStrict,
        NotEqualApprox,
        LessThan,
        LessOrEqual,
        GreaterThan,
        GreaterOrEqual,
    };

	struct Fraction
	{
        int32_t numerator;
        int32_t denominator;
	};
	using CompareOperand = std::variant<fig::fixed, fig::string, ContextLocator, Fraction>;

    class ComparisonCondition : public ConditionNode
    {
    public:
        ComparisonCondition(CompareOperand lhs, CompareOperand rhs, CompareOperator op);
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    private:
        CompareOperand _lhs;
        CompareOperand _rhs;
        CompareOperator _operator;
    };

    class FlagCondition : public ConditionNode
    {
    public:
        explicit FlagCondition(const ContextLocator& flag);
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    private:
        ContextLocator _flag;
    };

    class RandomCondition : public ConditionNode
    {
    public:
        explicit RandomCondition(int32_t num, int32_t denom);
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    private:
        int32_t _num;
        int32_t _denom;
    };

    class AlwaysCondition : public ConditionNode
    {
    public:
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    };

    class NeverCondition : public ConditionNode
    {
    public:
        bool Evaluate(const EvaluationArgs& eval) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    };

}

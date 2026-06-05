#ifndef CONDITION_NODE_H__
#define CONDITION_NODE_H__
#pragma once

#include "Figment.h"
#include "text/Context.h"

namespace fig
{
    class ConditionNode
    {
    public:
        virtual ~ConditionNode() = default;
        virtual bool Evaluate(const Context& context) const = 0;
    };

    using ConditionPtr = std::unique_ptr<ConditionNode>;

    class AndCondition : public ConditionNode
    {
    public:
        explicit AndCondition(std::vector<ConditionPtr> children);
        bool Evaluate(const Context& context) const override;

    private:
        std::vector<ConditionPtr> _children;
    };

    class OrCondition : public ConditionNode
    {
    public:
        explicit OrCondition(std::vector<ConditionPtr> children);
        bool Evaluate(const Context& context) const override;

    private:
        std::vector<ConditionPtr> _children;
    };

    class NotCondition : public ConditionNode
    {
    public:
        explicit NotCondition(ConditionPtr child);
        bool Evaluate(const Context& context) const override;

    private:
        ConditionPtr _child;
    };

    enum class CompareOperator
    {
        Equal,
        NotEqual,
        LessThan,
        LessOrEqual,
        GreaterThan,
        GreaterOrEqual,
    };

    using RhsValue = std::variant<float, fig::string>;

    class ComparisonCondition : public ConditionNode
    {
    public:
        ComparisonCondition(const fig::string& lhs, CompareOperator op, RhsValue rhs);
        bool Evaluate(const Context& context) const override;

    private:
        ContextLocation _location;
        CompareOperator _operator;
        RhsValue _rhs;
    };

    class FlagCondition : public ConditionNode
    {
    public:
        explicit FlagCondition(const fig::string& location);
        bool Evaluate(const Context& context) const override;

    private:
        ContextLocation _location;
    };

    enum class ConditionParseError
    {
        NoError,
        ParseError,
        UnexpectedToken,
        ExpectedOperand,
        ExpectedIdentifier,
        ExpectedParen,
        InvalidValue,
    };
}
#endif
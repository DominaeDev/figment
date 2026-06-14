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
        virtual std::unique_ptr<ConditionNode> Clone() const = 0;

        virtual explicit operator fig::string() const = 0;
    };

    using ConditionPtr = std::unique_ptr<ConditionNode>;

    class AndCondition : public ConditionNode
    {
    public:
        explicit AndCondition(std::vector<ConditionPtr> children);
        bool Evaluate(const Context& context) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;

    private:
        std::vector<ConditionPtr> _children;
    };

    class OrCondition : public ConditionNode
    {
    public:
        explicit OrCondition(std::vector<ConditionPtr> children);
        bool Evaluate(const Context& context) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    private:
        std::vector<ConditionPtr> _children;
    };

    class NotCondition : public ConditionNode
    {
    public:
        explicit NotCondition(ConditionPtr child);
        bool Evaluate(const Context& context) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
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

    using CompareOperand = std::variant<float, fig::string>;

    class ComparisonCondition : public ConditionNode
    {
    public:
        ComparisonCondition(CompareOperand lhs, CompareOperand rhs, CompareOperator op);
        bool Evaluate(const Context& context) const override;
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
        explicit FlagCondition(const fig::string& flag);
        explicit FlagCondition(const ContextLocator& flag);
        bool Evaluate(const Context& context) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    private:
        ContextLocator _flag;
    };

    class AlwaysCondition : public ConditionNode
    {
    public:
        bool Evaluate(const Context& context) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    };

    class NeverCondition : public ConditionNode
    {
    public:
        bool Evaluate(const Context& context) const override;
        ConditionPtr Clone() const override;

        explicit operator fig::string() const override;
    };

}
#endif
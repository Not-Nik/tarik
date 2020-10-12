#include <utility>

// Seno (c) Nikolas Wipper 2020

#ifndef SENO_EXPRESSION_H
#define SENO_EXPRESSION_H

// This also does precedence
enum ExprType {
    CALL_EXPR,
    DASH_EXPR, // add subtract
    DOT_EXPR, // multiply divide
    PREFIX_EXPR,
    ASSIGN_EXPR,
    NAME_EXPR,
    INT_EXPR
};

class Expression {
public:
    ExprType type;

    Expression() = default;

    explicit Expression(ExprType t) { type = t; }

    virtual std::string print() const { return ""; }
};

class NameExpression : public Expression {
    std::string name;
public:
    explicit NameExpression(std::string n) :
            Expression(NAME_EXPR),
            name(std::move(n)) { }

    std::string print() const override {
        return name;
    }
};

template <class NumberType>
class NumberExpression : public Expression {
    NumberType i;
public:
    explicit NumberExpression(const std::string & n) :
            Expression(INT_EXPR),
            i(std::stoi(n)) { }

    std::string print() const override {
        return std::to_string(i);
    }
};

using IntExpression = NumberExpression<long int>;
using RealExpression = NumberExpression<double>;

template <char pref>
class PrefixOperatorExpression : public Expression {
public:
    Expression * operand;

    explicit PrefixOperatorExpression(Expression * op) :
            Expression(PREFIX_EXPR),
            operand(op) {

    }

    std::string print() const override {
        return std::string({pref}) + operand->print();
    }
};

using PosExpression = PrefixOperatorExpression<'+'>;
using NegExpression = PrefixOperatorExpression<'-'>;

template <ExprType t, char pref>
class BinaryOperatorExpression : public Expression {
public:
    const Expression * left, * right;

    BinaryOperatorExpression(const Expression * l, const Expression * r) :
            Expression(t),
            left(l),
            right(r) {
    }

    std::string print() const override {
        return "(" + left->print() + std::string({pref}) + right->print() + ")";
    }
};

using AddExpression = BinaryOperatorExpression<DASH_EXPR, '+'>;
using SubExpression = BinaryOperatorExpression<DASH_EXPR, '-'>;
using MulExpression = BinaryOperatorExpression<DOT_EXPR, '*'>;
using DivExpression = BinaryOperatorExpression<DOT_EXPR, '/'>;

#endif //SENO_EXPRESSION_H

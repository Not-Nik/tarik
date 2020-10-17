// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_EXPRESSION_H
#define TARIK_EXPRESSION_H

#include <utility>

#include "Statements.h"

// This also does precedence
enum ExprType {
    CALL_EXPR,
    DASH_EXPR, // add subtract
    DOT_EXPR, // multiply divide
    PREFIX_EXPR,
    ASSIGN_EXPR,
    NAME_EXPR,
    INT_EXPR,
    REAL_EXPR
};

class Expression : public Statement {
public:
    ExprType expression_type;

    Expression() : Statement(), expression_type(NAME_EXPR) { }

    explicit Expression(ExprType t) : Statement(EXPR_STMT) { expression_type = t; }

    [[nodiscard]] virtual std::string print() const { return ""; }
};

class NameExpression : public Expression {
public:
    std::string name;

    explicit NameExpression(std::string n) :
            Expression(NAME_EXPR),
            name(std::move(n)) { }

    [[nodiscard]] std::string print() const override {
        return name;
    }
};

template <class NumberType, ExprType expr_type>
class NumberExpression : public Expression {
public:
    NumberType i;

    explicit NumberExpression(const std::string & n) :
            Expression(INT_EXPR),
            i(std::stoi(n)) { }

    [[nodiscard]] std::string print() const override {
        return std::to_string(i);
    }
};

using IntExpression = NumberExpression<long int, INT_EXPR>;
using RealExpression = NumberExpression<double, REAL_EXPR>;

template <char pref>
class PrefixOperatorExpression : public Expression {
public:
    Expression * operand;

    explicit PrefixOperatorExpression(Expression * op) :
            Expression(PREFIX_EXPR),
            operand(op) {

    }

    [[nodiscard]] std::string print() const override {
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

    [[nodiscard]] std::string print() const override {
        return "(" + left->print() + std::string({pref}) + right->print() + ")";
    }
};

using AddExpression = BinaryOperatorExpression<DASH_EXPR, '+'>;
using SubExpression = BinaryOperatorExpression<DASH_EXPR, '-'>;
using MulExpression = BinaryOperatorExpression<DOT_EXPR, '*'>;
using DivExpression = BinaryOperatorExpression<DOT_EXPR, '/'>;

class AssignExpression : public Expression {
public:
    VariableStatement * variable;
    Expression * value;

    AssignExpression(VariableStatement * var, Expression * val) :
            Expression(ASSIGN_EXPR),
            variable(var),
            value(val) {
    }

    [[nodiscard]] std::string print() const override {
        return "(" + variable->name + " = " + value->print() + ")";
    }
};

#endif //TARIK_EXPRESSION_H

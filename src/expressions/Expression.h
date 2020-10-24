// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_EXPRESSION_H
#define TARIK_EXPRESSION_H

#include <utility>

#include "Statements.h"

enum Precedence {
    ASSIGNMENT = 1,
    SUM,
    PRODUCT,
    PREFIX,
    CALL
};

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
            Expression(expr_type),
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

    ~PrefixOperatorExpression() override {
        delete operand;
    }

    [[nodiscard]] std::string print() const override {
        return std::string({pref}) + operand->print();
    }
};

using PosExpression = PrefixOperatorExpression<'+'>;
using NegExpression = PrefixOperatorExpression<'-'>;
using DerefExpression = PrefixOperatorExpression<'*'>;

template <ExprType t, char pref>
class BinaryOperatorExpression : public Expression {
public:
    const Expression * left, * right;

    BinaryOperatorExpression(const Expression * l, const Expression * r) :
            Expression(t),
            left(l),
            right(r) {
    }

    ~BinaryOperatorExpression() override {
        delete left;
        delete right;
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

    ~AssignExpression() override {
        delete value;
    }

    [[nodiscard]] std::string print() const override {
        return "(" + variable->name + " = " + value->print() + ")";
    }
};

class CallExpression : public Expression {
public:
    std::string function;
    std::vector<Expression *> arguments;

    CallExpression(std::string name, std::vector<Expression *> args) :
            Expression(CALL_EXPR),
            function(std::move(name)),
            arguments(std::move(args)) {
    }

    ~CallExpression() override {
        for (auto * arg : arguments) {
            delete arg;
        }
    }

    [[nodiscard]] std::string print() const override {
        std::string arg_string;
        for (auto arg : arguments) {
            arg_string += arg->print() + ", ";
        }
        if (!arg_string.empty()) {
            arg_string.pop_back();
            arg_string.pop_back();
        }
        return function + "(" + arg_string + ")";
    }
};

#endif //TARIK_EXPRESSION_H

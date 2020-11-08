// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_EXPRESSION_H
#define TARIK_EXPRESSION_H

#include <utility>

#include "Statements.h"

enum Precedence {
    ASSIGNMENT = 1, SUM, PRODUCT, PREFIX, CALL
};

enum ExprType {
    CALL_EXPR, DASH_EXPR, // add subtract
    DOT_EXPR, // multiply divide
    PREFIX_EXPR, ASSIGN_EXPR, NAME_EXPR, VARREF_EXPR, INT_EXPR, REAL_EXPR
};

class Expression : public Statement {
public:
    ExprType expression_type;

    Expression()
        : Statement(), expression_type(NAME_EXPR) {}

    explicit Expression(ExprType t)
        : Statement(EXPR_STMT, {}) { expression_type = t; }

    [[nodiscard]] virtual Type get_type() const = 0;
};

class NameExpression : public Expression {
public:
    std::string name;

    explicit NameExpression(std::string n)
        : Expression(NAME_EXPR), name(std::move(n)) {}

    [[nodiscard]] std::string print() const override {
        return name;
    }

    [[nodiscard]] Type get_type() const override {
        return {};
    }
};

class VariableReferenceExpression : public Expression {
public:
    VariableStatement *variable;

    explicit VariableReferenceExpression(VariableStatement *var)
        : Expression(VARREF_EXPR), variable(var) {}

    [[nodiscard]] std::string print() const override {
        return variable->name;
    }

    [[nodiscard]] Type get_type() const override {
        return variable->type;
    }
};

template <class NumberType, ExprType expr_type>
class NumberExpression : public Expression {
public:
    NumberType i;

    explicit NumberExpression(const std::string &n)
        : Expression(expr_type), i(std::stoi(n)) {}

    [[nodiscard]] std::string print() const override {
        return std::to_string(i);
    }

    [[nodiscard]] Type get_type() const override {
        if (expr_type == REAL_EXPR) {
            return Type(TypeUnion{.size = F64}, true, 0);
        } else if (expr_type == INT_EXPR) {
            return Type(TypeUnion{.size = I64}, true, 0);
        }
    }
};

using IntExpression = NumberExpression<long long int, INT_EXPR>;
using RealExpression = NumberExpression<double, REAL_EXPR>;

template <char pref>
class PrefixOperatorExpression : public Expression {
public:
    Expression *operand;

    explicit PrefixOperatorExpression(Expression *op)
        : Expression(PREFIX_EXPR), operand(op) {

    }

    ~PrefixOperatorExpression() override {
        delete operand;
    }

    [[nodiscard]] std::string print() const override {
        return std::string({pref}) + operand->print();
    }

    [[nodiscard]] Type get_type() const override {
        return operand->get_type();
    }
};

using PosExpression = PrefixOperatorExpression<'+'>;
using NegExpression = PrefixOperatorExpression<'-'>;
using DerefExpression = PrefixOperatorExpression<'*'>;

template <ExprType t, char pref>
class BinaryOperatorExpression : public Expression {
public:
    const Expression *left, *right;

    BinaryOperatorExpression(const Expression *l, const Expression *r)
        : Expression(t), left(l), right(r) {
    }

    ~BinaryOperatorExpression() override {
        delete left;
        delete right;
    }

    [[nodiscard]] std::string print() const override {
        return "(" + left->print() + std::string({pref}) + right->print() + ")";
    }

    [[nodiscard]] Type get_type() const override {
        return left->get_type();
    }
};

using AddExpression = BinaryOperatorExpression<DASH_EXPR, '+'>;
using SubExpression = BinaryOperatorExpression<DASH_EXPR, '-'>;
using MulExpression = BinaryOperatorExpression<DOT_EXPR, '*'>;
using DivExpression = BinaryOperatorExpression<DOT_EXPR, '/'>;

class AssignExpression : public Expression {
public:
    VariableStatement *variable;
    Expression *value;

    AssignExpression(VariableStatement *var, Expression *val)
        : Expression(ASSIGN_EXPR), variable(var), value(val) {
    }

    ~AssignExpression() override {
        delete value;
    }

    [[nodiscard]] std::string print() const override {
        return "(" + variable->name + " = " + value->print() + ")";
    }

    [[nodiscard]] Type get_type() const override {
        return variable->type;
    }
};

class CallExpression : public Expression {
public:
    FuncStatement* function;
    std::vector<Expression *> arguments;

    CallExpression(FuncStatement* func, std::vector<Expression *> args)
        : Expression(CALL_EXPR), function(func), arguments(std::move(args)) {
    }

    ~CallExpression() override {
        for (auto *arg : arguments) {
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
        return function->name + "(" + arg_string + ")";
    }

    [[nodiscard]] Type get_type() const override {
        return function->return_type;
    }
};

#endif //TARIK_EXPRESSION_H

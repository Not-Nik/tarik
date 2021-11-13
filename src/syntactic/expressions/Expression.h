// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_EXPRESSION_H_
#define TARIK_EXPRESSION_H_

#include <utility>

#include "Statements.h"

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

template <class NumberType, ExprType expr_type>
class NumberExpression : public Expression {
public:
    NumberType n;

    explicit NumberExpression(const std::string &n)
        : Expression(expr_type), n(std::stoi(n)) {}

    [[nodiscard]] std::string print() const override {
        return std::to_string(n);
    }

    [[nodiscard]] Type get_type() const override {
        if (expr_type == REAL_EXPR) {
            return Type(TypeUnion{F64}, true, 0);
        } else if (expr_type == INT_EXPR) {
            return Type(TypeUnion{I64}, true, 0);
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
using NotExpression = PrefixOperatorExpression<'!'>;

enum BinOpType {
    ADD, SUB, MUL, DIV, EQ, NEQ, SM, GR, SME, GRE,
};

constexpr ExprType to_expr_type(BinOpType bot) {
    switch (bot) {
        case ADD:
        case SUB:
            return DASH_EXPR;
        case MUL:
        case DIV:
            return DOT_EXPR;
        case EQ:
        case NEQ:
            return EQ_EXPR;
        case SM:
        case GR:
        case SME:
        case GRE:
            return COMP_EXPR;
    }
}

inline std::string to_string(BinOpType bot) {
    switch (bot) {
        case ADD:
            return "+";
        case SUB:
            return "-";
        case MUL:
            return "*";
        case DIV:
            return "/";
        case EQ:
            return "==";
        case NEQ:
            return "!=";
        case SM:
            return "<";
        case GR:
            return ">";
        case SME:
            return "<=";
        case GRE:
            return ">=";
    }
}

class BinaryOperatorExpression : public Expression {
public:
    BinOpType bin_op_type;
    Expression *left, *right;

    BinaryOperatorExpression(BinOpType bot, Expression *l, Expression *r)
        : Expression(to_expr_type(bot)), bin_op_type(bot), left(l), right(r) {
    }

    ~BinaryOperatorExpression() override {
        delete left;
        delete right;
    }

    [[nodiscard]] std::string print() const override {
        return "(" + left->print() + to_string(bin_op_type) + right->print() + ")";
    }

    [[nodiscard]] Type get_type() const override {
        return left->get_type();
    }
};

class AssignExpression : public Expression {
public:
    Expression *variable, *value;

    AssignExpression(Expression *var, Expression *val)
        : Expression(ASSIGN_EXPR), variable(var), value(val) {
    }

    ~AssignExpression() override {
        delete value;
        delete variable;
    }

    [[nodiscard]] std::string print() const override {
        return "(" + variable->print() + " = " + value->print() + ")";
    }

    [[nodiscard]] Type get_type() const override {
        return variable->get_type();
    }
};

class CallExpression : public Expression {
public:
    Expression *callee;
    std::vector<Expression *> arguments;

    CallExpression(Expression *c, std::vector<Expression *> args)
        : Expression(CALL_EXPR), callee(c), arguments(std::move(args)) {
    }

    ~CallExpression() override {
        for (auto *arg: arguments) {
            delete arg;
        }
        delete callee;
    }

    [[nodiscard]] std::string print() const override {
        std::string arg_string;
        for (auto arg: arguments) {
            arg_string += arg->print() + ", ";
        }
        if (!arg_string.empty()) {
            arg_string.pop_back();
            arg_string.pop_back();
        }
        return callee->print() + "(" + arg_string + ")";
    }

    [[nodiscard]] Type get_type() const override {
        // Todo: this is utterly wrong, but i don't think it'll ever get called anyways
        return callee->get_type();
    }
};

#endif //TARIK_EXPRESSION_H_

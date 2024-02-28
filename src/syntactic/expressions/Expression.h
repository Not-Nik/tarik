// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_EXPRESSION_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_EXPRESSION_H_

#include <utility>

#include "Statements.h"

class NameExpression : public Expression {
public:
    std::string name;

    explicit NameExpression(const LexerRange &lp, std::string n)
        : Expression(NAME_EXPR, lp),
          name(std::move(n)) {}

    [[nodiscard]] std::string print() const override {
        return name;
    }
};

template <class To, class = decltype(std::string(std::declval<To>()))>
std::string smart_cast_from_string(const std::string &n) {
    return n;
}

template <class To, class = decltype(To(std::stoi(std::declval<std::string>())))>
To smart_cast_from_string(const std::string &n) {
    return std::stoi(n);
}

template <>
inline bool smart_cast_from_string<bool>(const std::string &n) {
    if (n == "true")
        return true;
    if (n == "false")
        return false;
    throw "__unexpected_bool";
}

template <class To, class = decltype(std::to_string(std::declval<To>()))>
std::string smart_cast_to_string(To t) {
    return std::to_string(t);
}

inline std::string smart_cast_to_string(std::string s) {
    return s;
}

inline std::string smart_cast_to_string(bool s) {
    return s ? "true" : "false";
}

template <class PrimitiveType, ExprType expr_type>
class PrimitiveExpression : public Expression {
public:
    PrimitiveType n;

    PrimitiveExpression(LexerRange lp, const std::string &n)
        : Expression(expr_type, lp),
          n(smart_cast_from_string<PrimitiveType>(n)) {}

    [[nodiscard]] std::string print() const override {
        return smart_cast_to_string(n);
    }
};

using IntExpression = PrimitiveExpression<long long int, INT_EXPR>;
using BoolExpression = PrimitiveExpression<bool, BOOL_EXPR>;
using RealExpression = PrimitiveExpression<double, REAL_EXPR>;
using StringExpression = PrimitiveExpression<std::string, STR_EXPR>;

class NullExpression : public Expression {
public:
    explicit NullExpression(LexerRange lp)
        : Expression(NULL_EXPR, lp) {}

    [[nodiscard]] std::string print() const override {
        return "null";
    }
};

enum PrefixType {
    NEG,
    REF,
    DEREF,
    LOG_NOT,
    GLOBAL
};

inline std::string to_string(PrefixType pt) {
    switch (pt) {
        case NEG:
            return "-";
        case REF:
            return "&";
        case DEREF:
            return "*";
        case LOG_NOT:
            return "!";
        case GLOBAL:
            return "::";
    }
}

class PrefixOperatorExpression : public Expression {
public:
    PrefixType prefix_type;
    Expression *operand;

    explicit PrefixOperatorExpression(const LexerRange &lp, PrefixType pt, Expression *op)
        : Expression(PREFIX_EXPR, lp),
          prefix_type(pt),
          operand(op) {}

    ~PrefixOperatorExpression() override {
        delete operand;
    }

    [[nodiscard]] std::string print() const override {
        return to_string(prefix_type) + operand->print();
    }
};

enum BinOpType {
    PATH,
    ADD,
    SUB,
    MUL,
    DIV,
    EQ,
    NEQ,
    SM,
    GR,
    SME,
    GRE,
    MEM_ACC,
    ASSIGN
};

constexpr ExprType to_expr_type(BinOpType bot) {
    switch (bot) {
        case PATH:
            return PATH_EXPR;
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
        case MEM_ACC:
            return MEM_ACC_EXPR;
        case ASSIGN:
            return ASSIGN_EXPR;
    }
}

inline std::string to_string(BinOpType bot) {
    switch (bot) {
        case PATH:
            return "::";
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
        case MEM_ACC:
            return ".";
        case ASSIGN:
            return "=";
    }
}

class BinaryOperatorExpression : public Expression {
public:
    BinOpType bin_op_type;
    Expression *left, *right;

    BinaryOperatorExpression(const LexerRange &lp, BinOpType bot, Expression *l, Expression *r)
        : Expression(to_expr_type(bot), l->origin + r->origin),
          bin_op_type(bot),
          left(l),
          right(r) {}

    ~BinaryOperatorExpression() override {
        delete left;
        delete right;
    }

    [[nodiscard]] std::string print() const override {
        return "(" + left->print() + to_string(bin_op_type) + right->print() + ")";
    }
};

class CallExpression : public Expression {
public:
    Expression *callee;
    std::vector<Expression *> arguments;

    CallExpression(const LexerRange &lp, Expression *c, std::vector<Expression *> args)
        : Expression(CALL_EXPR, lp),
          callee(c),
          arguments(std::move(args)) {}

    ~CallExpression() override {
        for (auto *arg : arguments) {
            delete arg;
        }
        delete callee;
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
        return callee->print() + "(" + arg_string + ")";
    }
};

class EmptyExpression : public Expression {
public:
    EmptyExpression(const LexerRange &lp)
        : Expression(EMPTY_EXPR, lp) {
        type = Type(VOID);
    }

    [[nodiscard]] std::string print() const override {
        return "empty";
    }
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_EXPRESSION_H_

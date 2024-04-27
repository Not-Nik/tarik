// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_EXPRESSIONS_EXPRESSION_H_
#define TARIK_SRC_SEMANTIC_EXPRESSIONS_EXPRESSION_H_

#include "Base.h"
#include "syntactic/Types.h"

namespace aast
{

class NameExpression : public Expression {
public:
    std::string name;

    explicit NameExpression(const LexerRange &lp, Type type, std::string n)
        : Expression(NAME_EXPR, lp, type),
          name(std::move(n)) {}

    [[nodiscard]] std::string print() const override {
        return name;
    }

    bool flattens_to_member_access() const override {
        return true;
    }

    std::string flatten_to_member_access() const override {
        return name;
    }
};

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

template <class PrimitiveType, TypeSize size, int pointer_level, ExprType expr_type>
class PrimitiveExpression : public Expression {
public:
    PrimitiveType n;

    PrimitiveExpression(LexerRange lp, PrimitiveType n)
        : Expression(expr_type, lp, Type(size, pointer_level)),
          n(n) {}

    [[nodiscard]] std::string print() const override {
        return smart_cast_to_string(n);
    }
};

using IntExpression = PrimitiveExpression<long long int, U0, 0, INT_EXPR>;
using BoolExpression = PrimitiveExpression<bool, BOOL, 0, BOOL_EXPR>;
using RealExpression = PrimitiveExpression<double, F32, 0, REAL_EXPR>;
using StringExpression = PrimitiveExpression<std::string, U8, 1, STR_EXPR>;

enum PrefixType {
    NEG,
    REF,
    DEREF,
    LOG_NOT,
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
    }
}

class PrefixOperatorExpression : public Expression {
public:
    PrefixType prefix_type;
    Expression *operand;

    explicit PrefixOperatorExpression(const LexerRange &lp, Type type, PrefixType pt, Expression *op)
        : Expression(PREFIX_EXPR, lp, type),
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

    BinaryOperatorExpression(const LexerRange &lp, Type type, BinOpType bot, Expression *l, Expression *r)
        : Expression(to_expr_type(bot), l->origin + r->origin, type),
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

    bool flattens_to_member_access() const override {
        return (bin_op_type == MEM_ACC)
               ? (left->flattens_to_member_access() && right->flattens_to_member_access())
               : false;
    }

    std::string flatten_to_member_access() const override {
        return left->flatten_to_member_access() + "." + right->flatten_to_member_access();
    }
};

class CallExpression : public Expression {
public:
    Expression *callee;
    std::vector<Expression *> arguments;

    CallExpression(const LexerRange &lp, Type type, Expression *c, std::vector<Expression *> args)
        : Expression(CALL_EXPR, lp, type),
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
} // namespace ast

#endif //TARIK_SRC_SEMANTIC_EXPRESSIONS_EXPRESSION_H_

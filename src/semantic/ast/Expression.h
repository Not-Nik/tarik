// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_EXPRESSIONS_EXPRESSION_H_
#define TARIK_SRC_SEMANTIC_EXPRESSIONS_EXPRESSION_H_

#include <utility>

#include "Base.h"
#include "Statements.h"
#include "syntactic/Types.h"

namespace aast
{
class NameExpression : public Expression {
public:
    std::string name;

    explicit NameExpression(const LexerRange &lp, std::string n)
        : Expression(NAME_EXPR, lp, Type()),
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

class VariableExpression : public Expression {
public:
    class VariableStatement *var;

    explicit VariableExpression(const LexerRange &lp, VariableStatement *var, std::vector<Statement *> p = {})
        : Expression(VAR_EXPR, lp, var->type, p),
          var(var) {}

    [[nodiscard]] std::string print() const override {
        return var->name.raw;
    }

    bool flattens_to_member_access() const override {
        return true;
    }

    std::string flatten_to_member_access() const override {
        return var->name.raw;
    }
};

template <class To, class = decltype(std::to_string(std::declval<To>()))>
std::string smart_cast_to_string(To t) {
    return std::to_string(t);
}

inline std::string smart_cast_to_string(std::string s) {
    return '"' + s + '"';
}

inline std::string smart_cast_to_string(bool s) {
    return s ? "true" : "false";
}

template <class PrimitiveType, TypeSize size, int pointer_level, ExprType expr_type>
class PrimitiveExpression : public Expression {
public:
    PrimitiveType n;

    PrimitiveExpression(const LexerRange &lp, PrimitiveType n)
        : Expression(expr_type, lp, Type(size, pointer_level)),
          n(n) {}

    [[nodiscard]] std::string print() const override {
        return smart_cast_to_string(n);
    }
};

using IntExpression = PrimitiveExpression<long long int, U0, 0, INT_EXPR>;
using BoolExpression = PrimitiveExpression<bool, BOOL, 0, BOOL_EXPR>;
using RealExpression = PrimitiveExpression<double, F32, 0, REAL_EXPR>;
using StringExpression = PrimitiveExpression<std::string, STR, 1, STR_EXPR>;

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
        default:
            std::unreachable();
    }
}

class PrefixExpression : public Expression {
public:
    PrefixType prefix_type;
    Expression *operand;

    explicit PrefixExpression(const LexerRange &lp, const Type &type, PrefixType pt, Expression *op)
        : Expression(PREFIX_EXPR, lp, std::move(type)),
          prefix_type(pt),
          operand(op) {}

    ~PrefixExpression() override {
        delete operand;
    }

    [[nodiscard]] std::string print() const override {
        return to_string(prefix_type) + operand->print();
    }

    Expression *get_inner() override {
        return operand;
    }

    std::vector<Statement *> collect_prelude() override {
        return operand->collect_prelude();
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

class BinaryExpression : public Expression {
public:
    BinOpType bin_op_type;
    Expression *left, *right;

    BinaryExpression(const LexerRange &, const Type &type, BinOpType bot, Expression *l, Expression *r)
        : Expression(to_expr_type(bot), l->origin + r->origin, type),
          bin_op_type(bot),
          left(l),
          right(r) {}

    ~BinaryExpression() override {
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

    std::vector<Statement *> collect_prelude() override {
        std::vector<Statement *> prel = left->collect_prelude();
        std::vector<Statement *> prel2 = right->collect_prelude();

        prel.insert(prel.end(), prel2.begin(), prel2.end());
        return prel;
    }
};

class CastExpression : public Expression {
public:
    Expression *expression;

    CastExpression(const LexerRange &lp, const Type &target_type, Expression *expr)
        : Expression(CAST_EXPR, lp, target_type),
          expression(expr) {}

    ~CastExpression() override {
        delete expression;
    }

    [[nodiscard]] std::string print() const override {
        return "as!(" + expression->print() + ", " + type.str() + ")";
    }

    std::vector<Statement *> collect_prelude() override {
        return expression->collect_prelude();
    }
};

class CallExpression : public Expression {
public:
    Expression *callee;
    std::vector<Expression *> arguments;

    CallExpression(const LexerRange &lp, const Type &type, Expression *c, std::vector<Expression *> args)
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
        for (auto *arg : arguments) {
            arg_string += arg->print() + ", ";
        }
        if (!arg_string.empty()) {
            arg_string.pop_back();
            arg_string.pop_back();
        }
        return callee->print() + "(" + arg_string + ")";
    }

    std::vector<Statement *> collect_prelude() override {
        std::vector<Statement *> prel = callee->collect_prelude();

        for (auto arg : arguments) {
            std::vector<Statement *> arg_prel = arg->collect_prelude();
            prel.insert(prel.end(), arg_prel.begin(), arg_prel.end());
        }

        return prel;
    }
};
} // namespace ast

#endif //TARIK_SRC_SEMANTIC_EXPRESSIONS_EXPRESSION_H_

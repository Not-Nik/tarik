// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_EXPRESSION_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_EXPRESSION_H_

#include <utility>

#include "Statements.h"

class NameExpression : public Expression {
public:
    std::string name;

    explicit NameExpression(LexerPos lp, std::string n)
        : Expression(NAME_EXPR, std::move(lp)), name(std::move(n)) {}

    [[nodiscard]] std::string print() const override {
        return name;
    }

    [[nodiscard]] Type get_type() const override {
        return {};
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

template <class To, class = decltype(std::to_string(std::declval<To>()))>
std::string smart_cast_to_string(To t) {
    return std::to_string(t);
}

inline std::string smart_cast_to_string(std::string s) {
    return s;
}

template <class PrimitiveType, ExprType expr_type>
class PrimitiveExpression : public Expression {
public:
    PrimitiveType n;

    explicit PrimitiveExpression(LexerPos lp, const std::string &n)
        : Expression(expr_type, lp), n(smart_cast_from_string<PrimitiveType>(n)) {}

    [[nodiscard]] std::string print() const override {
        return smart_cast_to_string(n);
    }

    [[nodiscard]] Type get_type() const override {
        if (expr_type == REAL_EXPR) {
            return Type(F64, 0);
        } else if (expr_type == INT_EXPR) {
            return Type(I64, 0);
        } else if (expr_type == STR_EXPR) {
            return Type(U8, 1);
        }
    }
};

using IntExpression = PrimitiveExpression<long long int, INT_EXPR>;
using RealExpression = PrimitiveExpression<double, REAL_EXPR>;
using StringExpression = PrimitiveExpression<std::string, STR_EXPR>;

enum PrefixType {
    POS, NEG, DEREF, LOG_NOT
};

inline std::string to_string(PrefixType pt) {
    switch (pt) {
        case POS:
            return "+";
        case NEG:
            return "-";
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

    explicit PrefixOperatorExpression(LexerPos lp, PrefixType pt, Expression *op)
        : Expression(PREFIX_EXPR, std::move(lp)), prefix_type(pt), operand(op) {
    }

    ~PrefixOperatorExpression() override {
        delete operand;
    }

    [[nodiscard]] std::string print() const override {
        return to_string(prefix_type) + operand->print();
    }

    [[nodiscard]] Type get_type() const override {
        return operand->get_type();
    }
};

enum BinOpType {
    ADD, SUB, MUL, DIV, EQ, NEQ, SM, GR, SME, GRE, ASSIGN
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
        case ASSIGN:
            return "=";
    }
}

class BinaryOperatorExpression : public Expression {
public:
    BinOpType bin_op_type;
    Expression *left, *right;

    BinaryOperatorExpression(LexerPos lp, BinOpType bot, Expression *l, Expression *r)
        : Expression(to_expr_type(bot), std::move(lp)), bin_op_type(bot), left(l), right(r) {
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

class CallExpression : public Expression {
public:
    Expression *callee;
    std::vector<Expression *> arguments;

    CallExpression(LexerPos lp, Expression *c, std::vector<Expression *> args)
        : Expression(CALL_EXPR, std::move(lp)), callee(c), arguments(std::move(args)) {
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

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_EXPRESSION_H_

// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_PARSLETS_H
#define TARIK_PARSLETS_H

#include "Expression.h"
#include "../Token.h"
#include "../Parser.h"

class PrefixParselet {
public:
    virtual Expression * parse(Parser * parser, const Token & token) { return {}; }
};

class InfixParselet {
public:
    virtual Expression * parse(Parser * parser, const Expression * left, const Token & token) { return {}; }

    virtual Precedence get_type() { return static_cast<Precedence>(0); }
};

template <class SimpleExpression>
class SimpleParselet : public PrefixParselet {
    Expression * parse(Parser * parser, const Token & token) override {
        return (Expression *) new SimpleExpression(token.raw);
    }
};

using NameParselet = SimpleParselet<NameExpression>;
using IntParselet = SimpleParselet<IntExpression>;
using RealParselet = SimpleParselet<RealExpression>;

#define TEST_VARIABLE_PRIMITIVE_EXPR(name) \
        if (name->expression_type == NAME_EXPR) { \
            VariableStatement * var = parser->require_var(name->print()); \
            if (var) { \
                Type t = var->type; \
                parser->iassert(t.is_primitive || t.pointer_level > 0, "Invalid operand to binary expression"); \
            } \
        }

template <class OperatorExpression>
class PrefixOperatorParselet : public PrefixParselet {
    Expression * parse(Parser * parser, const Token & token) override {
        Expression * right = parser->parse_expression(PREFIX);

        TEST_VARIABLE_PRIMITIVE_EXPR(right)

        return new OperatorExpression(right);
    }
};

using PosParselet = PrefixOperatorParselet<PosExpression>;
using NegParselet = PrefixOperatorParselet<NegExpression>;
using DerefParselet = PrefixOperatorParselet<DerefExpression>;

template <class OperatorExpression, Precedence prec>
class BinaryOperatorParselet : public InfixParselet {
    Expression * parse(Parser * parser, const Expression * left, const Token & token) override {
        Expression * right = parser->parse_expression(prec);

        TEST_VARIABLE_PRIMITIVE_EXPR(left)
        TEST_VARIABLE_PRIMITIVE_EXPR(right)

        return new OperatorExpression(left, right);
    }

    Precedence get_type() override {
        return prec;
    }
};

using AddParselet = BinaryOperatorParselet<AddExpression, SUM>;
using SubParselet = BinaryOperatorParselet<SubExpression, SUM>;
using MulParselet = BinaryOperatorParselet<MulExpression, PRODUCT>;
using DivParselet = BinaryOperatorParselet<DivExpression, PRODUCT>;

class GroupParselet : public PrefixParselet {
    Expression * parse(Parser * parser, const Token & token) override {
        Expression * e = parser->parse_expression();
        parser->expect(PAREN_CLOSE);
        return e;
    }
};

class AssignParselet : public InfixParselet {
    Expression * parse(Parser * parser, const Expression * left, const Token & token) override {
        Expression * right = parser->parse_expression(ASSIGNMENT - 1);

        parser->iassert(left->expression_type == NAME_EXPR, "Can't assign to expression");

        std::string var_name = left->print();
        delete left;
        return new AssignExpression(parser->require_var(var_name), right);
    }

    Precedence get_type() override {
        return ASSIGNMENT;
    }
};

class CallParselet : public InfixParselet {
    Expression * parse(Parser * parser, const Expression * left, const Token & token) override {
        parser->iassert(left->expression_type == NAME_EXPR, "Can't call expression");
        FuncStatement * func = parser->require_func(left->print());
        std::vector<Expression *> args;

        while (!parser->lexer.peek().raw.empty() && parser->lexer.peek().id != PAREN_CLOSE) {
            args.push_back(parser->parse_expression());
            if (args.back()->expression_type == NAME_EXPR)
                parser->require_var(args.back()->print());

            if (parser->lexer.peek().id != PAREN_CLOSE)
                parser->expect(COMMA);
        }

        std::string name = left->print();
        delete left;

        return new CallExpression(name, args);
    }

    Precedence get_type() override {
        return CALL;
    }
};

#endif //TARIK_PARSLETS_H

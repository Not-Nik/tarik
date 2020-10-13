// Seno (c) Nikolas Wipper 2020

#ifndef SENO_PARSLETS_H
#define SENO_PARSLETS_H

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

    virtual ExprType get_type() { return static_cast<ExprType>(0); }
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

template <class OperatorExpression>
class PrefixOperatorParselet : public PrefixParselet {
    Expression * parse(Parser * parser, const Token & token) override {
        return new OperatorExpression(parser->parse_expression(PREFIX_EXPR));
    }
};

using PosParselet = PrefixOperatorParselet<PosExpression>;
using NegParselet = PrefixOperatorParselet<NegExpression>;

template <class OperatorExpression, ExprType prec>
class BinaryOperatorParselet : public InfixParselet {
    Expression * parse(Parser * parser, const Expression * left, const Token & token) override {
        return new OperatorExpression(left, parser->parse_expression(prec));
    }

    ExprType get_type() override {
        return prec;
    }
};

using AddParselet = BinaryOperatorParselet<AddExpression, DASH_EXPR>;
using SubParselet = BinaryOperatorParselet<SubExpression, DASH_EXPR>;
using MulParselet = BinaryOperatorParselet<MulExpression, DOT_EXPR>;
using DivParselet = BinaryOperatorParselet<DivExpression, DOT_EXPR>;

class GroupParselet : public PrefixParselet {
    Expression * parse(Parser * parser, const Token & token) override {
        Expression * e = parser->parse_expression();
        parser->expect(PAREN_CLOSE);
        return e;
    }
};

#endif //SENO_PARSLETS_H

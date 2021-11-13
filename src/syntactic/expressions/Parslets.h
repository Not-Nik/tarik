// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_PARSLETS_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_PARSLETS_H_

#include "Expression.h"
#include "lexical/Token.h"
#include "syntactic/Parser.h"

class PrefixParselet {
public:
    virtual Expression *parse(Parser *, const Token &) = 0;

    virtual ~PrefixParselet() = default;
};

class InfixParselet {
public:
    virtual Expression *parse(Parser *parser, Expression *left) = 0;

    virtual Precedence get_type() { return static_cast<Precedence>(0); }
    virtual ~InfixParselet() = default;
};

template <class SimpleExpression>
class SimpleParselet : public PrefixParselet {
    Expression *parse(Parser *, const Token &token) override {
        return (Expression *) new SimpleExpression(token.raw);
    }
};

using IntParselet = SimpleParselet<IntExpression>;
using RealParselet = SimpleParselet<RealExpression>;

class NameParselet : public PrefixParselet {
    Expression *parse(Parser *, const Token &token) override {
        return new NameExpression(token.raw);
    }
};

template <class OperatorExpression>
class PrefixOperatorParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &) override {
        Expression *right = parser->parse_expression(PREFIX);

        return new OperatorExpression(right);
    }
};

using PosParselet = PrefixOperatorParselet<PosExpression>;
using NegParselet = PrefixOperatorParselet<NegExpression>;
using DerefParselet = PrefixOperatorParselet<DerefExpression>;
using NotParselet = PrefixOperatorParselet<NotExpression>;

template <BinOpType bot, Precedence prec>
class BinaryOperatorParselet : public InfixParselet {
    Expression *parse(Parser *parser, Expression *left) override {
        Expression *right = parser->parse_expression(prec);

        return new BinaryOperatorExpression(bot, left, right);
    }

    Precedence get_type() override {
        return prec;
    }
};

using AddParselet = BinaryOperatorParselet<ADD, SUM>;
using SubParselet = BinaryOperatorParselet<SUB, SUM>;
using MulParselet = BinaryOperatorParselet<MUL, PRODUCT>;
using DivParselet = BinaryOperatorParselet<DIV, PRODUCT>;
using EqParselet = BinaryOperatorParselet<EQ, EQUALITY>;
using NeqParselet = BinaryOperatorParselet<NEQ, EQUALITY>;
using SmParselet = BinaryOperatorParselet<SM, COMPARE>;
using GrParselet = BinaryOperatorParselet<GR, COMPARE>;
using SeParselet = BinaryOperatorParselet<SME, COMPARE>;
using GeParselet = BinaryOperatorParselet<GRE, COMPARE>;

class GroupParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &) override {
        Expression *e = parser->parse_expression();
        parser->expect(PAREN_CLOSE);
        return e;
    }
};

class AssignParselet : public InfixParselet {
    Expression *parse(Parser *parser, Expression *left) override {
        Expression *right = parser->parse_expression(ASSIGNMENT - 1);

        return new AssignExpression(left, right);
    }

    Precedence get_type() override {
        return ASSIGNMENT;
    }
};

class CallParselet : public InfixParselet {
    Expression *parse(Parser *parser, Expression *left) override {
        std::vector<Expression *> args;

        while (!parser->lexer.peek().raw.empty() && parser->lexer.peek().id != PAREN_CLOSE) {
            args.push_back(parser->parse_expression());

            if (parser->lexer.peek().id != PAREN_CLOSE)
                parser->expect(COMMA);
        }
        parser->lexer.consume();

        return new CallExpression(left, args);
    }

    Precedence get_type() override {
        return CALL;
    }
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_PARSLETS_H_
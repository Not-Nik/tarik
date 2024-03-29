// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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
    virtual Expression *parse(Parser *parser, const Token &, Expression *left) = 0;

    virtual Precedence get_type() { return static_cast<Precedence>(0); }
    virtual ~InfixParselet() = default;
};

class NullParselet : public PrefixParselet {
    Expression *parse(Parser *, const Token &token) override {
        return new NullExpression(token.origin);
    }
};

template <class SimpleExpression>
class SimpleParselet : public PrefixParselet {
    Expression *parse(Parser *, const Token &token) override {
        return (Expression *) new SimpleExpression(token.origin, token.raw);
    }
};

using IntParselet = SimpleParselet<IntExpression>;
using BoolParselet = SimpleParselet<BoolExpression>;
using RealParselet = SimpleParselet<RealExpression>;
using StringParselet = SimpleParselet<StringExpression>;

class NameParselet : public PrefixParselet {
    Expression *parse(Parser *, const Token &token) override {
        return new NameExpression(token.origin, token.raw);
    }
};

template <PrefixType prefix_type>
class PrefixOperatorParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &token) override {
        Expression *right = parser->parse_expression(PREFIX);

        return new PrefixOperatorExpression(token.origin, prefix_type, right);
    }
};

using NegParselet = PrefixOperatorParselet<NEG>;
using RefParselet = PrefixOperatorParselet<REF>;
using DerefParselet = PrefixOperatorParselet<DEREF>;
using NotParselet = PrefixOperatorParselet<LOG_NOT>;
using GlobalParselet = PrefixOperatorParselet<GLOBAL>;

template <BinOpType bot, Precedence prec>
class BinaryOperatorParselet : public InfixParselet {
    Expression *parse(Parser *parser, const Token &token, Expression *left) override {
        Expression *right = parser->parse_expression(prec);

        return new BinaryOperatorExpression(token.origin, bot, left, right);
    }

    Precedence get_type() override {
        return prec;
    }
};

using PathParselet = BinaryOperatorParselet<PATH, NAME_CONCAT>;
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
using MemberAccessParselet = BinaryOperatorParselet<MEM_ACC, CALL>;

class GroupParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &) override {
        Expression *e = parser->parse_expression();
        parser->expect(PAREN_CLOSE);
        return e;
    }
};

class AssignParselet : public InfixParselet {
    Expression *parse(Parser *parser, const Token &token, Expression *left) override {
        Expression *right = parser->parse_expression(ASSIGNMENT - 1);

        return new BinaryOperatorExpression(token.origin, ASSIGN, left, right);
    }

    Precedence get_type() override {
        return ASSIGNMENT;
    }
};

class CallParselet : public InfixParselet {
    Expression *parse(Parser *parser, const Token &token, Expression *left) override {
        std::vector<Expression *> args;

        while (parser->lexer.peek().id != END && parser->lexer.peek().id != PAREN_CLOSE) {
            args.push_back(parser->parse_expression());

            if (parser->lexer.peek().id != PAREN_CLOSE)
                parser->expect(COMMA);
        }
        parser->lexer.consume();

        return new CallExpression(token.origin, left, args);
    }

    Precedence get_type() override {
        return CALL;
    }
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_PARSLETS_H_

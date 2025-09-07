// tarik (c) Nikolas Wipper 2020-2024

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
    virtual ast::Expression *parse(Parser *, const Token &) = 0;

    virtual ~PrefixParselet() = default;
};

class InfixParselet {
public:
    virtual ast::Expression *parse(Parser *parser, const Token &, ast::Expression *left) = 0;

    virtual bool can_parse(ast::Expression *) { return true; }
    virtual ast::Precedence get_type() { return static_cast<ast::Precedence>(0); }
    virtual ~InfixParselet() = default;
};

template <class SimpleExpression>
class SimpleParselet : public PrefixParselet {
    ast::Expression *parse(Parser *, const Token &token) override {
        return (ast::Expression *) new SimpleExpression(token.origin, token.raw);
    }
};

using IntParselet = SimpleParselet<ast::IntExpression>;
using BoolParselet = SimpleParselet<ast::BoolExpression>;
using RealParselet = SimpleParselet<ast::RealExpression>;
using StringParselet = SimpleParselet<ast::StringExpression>;

using NameParselet = SimpleParselet<ast::NameExpression>;
using MacroNameParselet = SimpleParselet<ast::MacroNameExpression>;


template <ast::PrefixType prefix_type, ast::Precedence precedence = ast::PREFIX>
class PrefixOperatorParselet : public PrefixParselet {
    ast::Expression *parse(Parser *parser, const Token &token) override {
        ast::Expression *right = parser->parse_expression(precedence);

        return new ast::PrefixExpression(token.origin, prefix_type, right);
    }
};

using NegParselet = PrefixOperatorParselet<ast::NEG>;
using RefParselet = PrefixOperatorParselet<ast::REF>;
using DerefParselet = PrefixOperatorParselet<ast::DEREF>;
using NotParselet = PrefixOperatorParselet<ast::LOG_NOT>;
using GlobalParselet = PrefixOperatorParselet<ast::GLOBAL, ast::NAME_CONCAT>;

template <ast::BinOpType bot, ast::Precedence prec>
class BinaryOperatorParselet : public InfixParselet {
    ast::Expression *parse(Parser *parser, const Token &token, ast::Expression *left) override {
        ast::Expression *right = parser->parse_expression(prec);

        return new ast::BinaryExpression(token.origin, bot, left, right);
    }

    ast::Precedence get_type() override {
        return prec;
    }
};

using PathParselet = BinaryOperatorParselet<ast::PATH, ast::NAME_CONCAT>;
using AddParselet = BinaryOperatorParselet<ast::ADD, ast::SUM>;
using SubParselet = BinaryOperatorParselet<ast::SUB, ast::SUM>;
using MulParselet = BinaryOperatorParselet<ast::MUL, ast::PRODUCT>;
using DivParselet = BinaryOperatorParselet<ast::DIV, ast::PRODUCT>;
using EqParselet = BinaryOperatorParselet<ast::EQ, ast::EQUALITY>;
using NeqParselet = BinaryOperatorParselet<ast::NEQ, ast::EQUALITY>;
using SmParselet = BinaryOperatorParselet<ast::SM, ast::COMPARE>;
using GrParselet = BinaryOperatorParselet<ast::GR, ast::COMPARE>;
using SeParselet = BinaryOperatorParselet<ast::SME, ast::COMPARE>;
using GeParselet = BinaryOperatorParselet<ast::GRE, ast::COMPARE>;
using MemberAccessParselet = BinaryOperatorParselet<ast::MEM_ACC, ast::CALL>;

class GroupParselet : public PrefixParselet {
    ast::Expression *parse(Parser *parser, const Token &) override {
        ast::Expression *e = parser->parse_expression();
        parser->expect(PAREN_CLOSE);
        return e;
    }
};

class AssignParselet : public InfixParselet {
    ast::Expression *parse(Parser *parser, const Token &token, ast::Expression *left) override {
        ast::Expression *right = parser->parse_expression(ast::ASSIGNMENT - 1);

        return new ast::BinaryExpression(token.origin, ast::ASSIGN, left, right);
    }

    ast::Precedence get_type() override {
        return ast::ASSIGNMENT;
    }
};

class StructInitParselet : public InfixParselet {
    ast::Expression *parse(Parser *parser, const Token &, ast::Expression *left) override {
        std::vector<ast::Expression *> args;

        while (!parser->is_peek(END) && !parser->is_peek(BRACKET_CLOSE)) {
            args.push_back(parser->parse_expression());

            if (!parser->is_peek(COMMA))
                break;
            parser->expect(COMMA);
        }
        LexerRange origin = left->origin + parser->expect(BRACKET_CLOSE).origin;

        return new ast::StructInitExpression(origin, left, args);
    }

    bool can_parse(ast::Expression *left) override {
        return left->expression_type == ast::NAME_EXPR || left->expression_type == ast::PATH_EXPR;
    }

    ast::Precedence get_type() override {
        return ast::CALL;
    }
};

class CallParselet : public InfixParselet {
    ast::Expression *parse(Parser *parser, const Token &token, ast::Expression *left) override {
        if (left->expression_type == ast::MACRO_NAME_EXPR ||
            (left->expression_type == ast::MEM_ACC_EXPR &&
                ((ast::BinaryExpression *) left)->right->expression_type == ast::MACRO_NAME_EXPR))
            return parse_macro(parser, token, left);

        std::vector<ast::Expression *> args;

        while (!parser->is_peek(END) && !parser->is_peek(PAREN_CLOSE)) {
            args.push_back(parser->parse_expression());

            if (!parser->is_peek(COMMA))
                break;
            parser->expect(COMMA);
        }
        LexerRange origin = left->origin + parser->expect(PAREN_CLOSE).origin;

        return new ast::CallExpression(origin, left, args);
    }

    ast::Expression *parse_macro(Parser *parser, const Token &token, ast::Expression *left) {
        std::vector<ast::Expression *> args;

        while (!parser->is_peek(END) && !parser->is_peek(PAREN_CLOSE)) {
            Lexer::State state = parser->lexer.checkpoint();
            std::optional type = parser->type();

            if ((!parser->is_peek(PAREN_CLOSE) && !parser->is_peek(COMMA)) ||
                (type.has_value() && type.value().pointer_level == 0 && !type.value().is_primitive() &&
                    type.value().get_user().get_parts().size() == 1)) {
                // Either the argument isn't done or the argument was too simple to definitely be a type
                parser->lexer.rollback(state);
                type = {};
            }

            if (!type.has_value())
                args.push_back(parser->parse_expression());
            else
                args.push_back(new ast::TypeExpression(type.value(),
                                                       state.pos.as_zero_range() +
                                                       parser->lexer.checkpoint().pos.as_zero_range()));

            if (!parser->is_peek(COMMA))
                break;
            parser->expect(COMMA);
        }
        parser->expect(PAREN_CLOSE);

        return new ast::CallExpression(token.origin, left, args);
    }

    ast::Precedence get_type() override {
        return ast::CALL;
    }
};

class ListParselet : public PrefixParselet {
    ast::Expression *parse(Parser *parser, const Token &left) override {
        std::vector<ast::Expression *> elements;
        while (!parser->is_peek(BRACKET_CLOSE) && !parser->is_peek(END)) {
            elements.push_back(parser->parse_expression());

            if (!parser->is_peek(COMMA))
                break;
            parser->expect(COMMA);
        }
        Token close = parser->expect(BRACKET_CLOSE);

        return new ast::ListExpression(left.origin + close.origin, elements);
    }
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_PARSLETS_H_

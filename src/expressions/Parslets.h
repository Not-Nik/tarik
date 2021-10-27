// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_PARSLETS_H_
#define TARIK_PARSLETS_H_

#include "Expression.h"
#include "../Token.h"
#include "../Parser.h"

class PrefixParselet {
public:
    virtual Expression *parse(Parser *parser, const Token &token) { return {}; }

    virtual ~PrefixParselet() {}
};

class InfixParselet {
public:
    virtual Expression *parse(Parser *parser, const Expression *left, const Token &token) { return {}; }

    virtual Precedence get_type() { return static_cast<Precedence>(0); }
    virtual ~InfixParselet() {}
};

template <class SimpleExpression>
class SimpleParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &token) override {
        return (Expression *) new SimpleExpression(token.raw);
    }
};

using IntParselet = SimpleParselet<IntExpression>;
using RealParselet = SimpleParselet<RealExpression>;

class NameParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &token) override {
        if (parser->is_peek(PAREN_OPEN)) {
            return new NameExpression(token.raw);
        } else {
            return new VariableReferenceExpression(parser->require_var(token.raw));
        }
    }
};

#define TEST_VARIABLE_PRIMITIVE_EXPR(name) \
        if (name->expression_type == VARREF_EXPR) { \
            auto * var = (VariableReferenceExpression *)name; \
            if (var) { \
                Type t = var->get_type(); \
                parser->iassert(t.is_primitive || t.pointer_level > 0, "Invalid operand to binary expression"); \
            } \
        }

template <class OperatorExpression>
class PrefixOperatorParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &token) override {
        Expression *right = parser->parse_expression(PREFIX);;
        TEST_VARIABLE_PRIMITIVE_EXPR(right)

        return new OperatorExpression(right);
    }
};

using PosParselet = PrefixOperatorParselet<PosExpression>;
using NegParselet = PrefixOperatorParselet<NegExpression>;
using DerefParselet = PrefixOperatorParselet<DerefExpression>;
using NotParselet = PrefixOperatorParselet<NotExpression>;

template <class OperatorExpression, Precedence prec>
class BinaryOperatorParselet : public InfixParselet {
    Expression *parse(Parser *parser, const Expression *left, const Token &token) override {
        Expression *right = parser->parse_expression(prec);

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
using EqParselet = BinaryOperatorParselet<EqExpression, EQUALITY>;
using NeqParselet = BinaryOperatorParselet<NeqExpression, EQUALITY>;
using SmParselet = BinaryOperatorParselet<SmExpression, COMPARE>;
using GrParselet = BinaryOperatorParselet<GrExpression, COMPARE>;
using SeParselet = BinaryOperatorParselet<SeExpression, COMPARE>;
using GeParselet = BinaryOperatorParselet<GeExpression, COMPARE>;

class GroupParselet : public PrefixParselet {
    Expression *parse(Parser *parser, const Token &token) override {
        Expression *e = parser->parse_expression();
        parser->expect(PAREN_CLOSE);
        return e;
    }
};

class AssignParselet : public InfixParselet {
    Expression *parse(Parser *parser, const Expression *left, const Token &token) override {
        Expression *right = parser->parse_expression(ASSIGNMENT - 1);

        parser->iassert(left->expression_type == VARREF_EXPR, "Can't assign to expression");
        VariableStatement *var = ((VariableReferenceExpression *) left)->variable;
        delete left;
        if (var) {
            parser->iassert(var->type.is_compatible(right->get_type()), "Incompatible types for assignment");
        }

        return new AssignExpression(var, right);
    }

    Precedence get_type() override {
        return ASSIGNMENT;
    }
};

class CallParselet : public InfixParselet {
    Expression *parse(Parser *parser, const Expression *left, const Token &token) override {
        parser->iassert(left->expression_type == NAME_EXPR, "Can't call expression");
        FuncStatement *func = parser->require_func(left->print());
        std::vector<Expression *> args;

        size_t i = 0;

        while (!parser->lexer.peek().raw.empty() && parser->lexer.peek().id != PAREN_CLOSE) {
            args.push_back(parser->parse_expression());

            if (i < func->arguments.size()) {
                parser->iassert(args.back()->get_type().is_compatible(func->arguments[i]->type), "");
            }

            if (parser->lexer.peek().id != PAREN_CLOSE)
                parser->expect(COMMA);
        }
        parser->lexer.consume();

        parser->iassert(args.size() >= func->arguments.size(),
                        "Too few arguments, expected %i found %i.",
                        func->arguments.size(),
                        args.size());

        parser->iassert(args.size() <= func->arguments.size(),
                        "Too many arguments, expected %i found %i.",
                        func->arguments.size(),
                        args.size());

        delete left;

        return new CallExpression(func, args);
    }

    Precedence get_type() override {
        return CALL;
    }
};

#endif //TARIK_PARSLETS_H_

// Seno (c) Nikolas Wipper 2020

#include "Parser.h"

#include <comperr.h>

#include <utility>

void Parser::iassert(bool cond, const std::string & what, bool warn, ...) {
    va_list args;
    va_start(args, warn);
    vcomperr(cond, what.c_str(), warn, filename.c_str(), lexer.where().l, lexer.where().p, args);
    va_end(args);
}

ExprType Parser::get_precedence() {
    if (infix_parslets.count(lexer.peek().id) > 0)
        return infix_parslets[lexer.peek().id]->get_type();
    return static_cast<ExprType>(0);
}

Parser::Parser(std::string code, std::string fn) :
        lexer(std::move(code)),
        filename(std::move(fn)) { }

Expression Parser::parser_expression(int precedence) {
    Token token = lexer.consume();
    iassert(prefix_parslets.count(token.id) > 0, "Could not parser %s", token.raw.c_str());
    PrefixParselet * prefix = prefix_parslets[token.id];

    Expression left = prefix->parse(this, token);

    while (precedence < get_precedence()) {
        token = lexer.consume();

        InfixParselet * infix = infix_parslets[token.id];
        left = infix->parse(this, left, token);
    }

    return left;
}

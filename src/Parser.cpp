// Seno (c) Nikolas Wipper 2020

#include "Parser.h"

#include "expressions/Parslets.h"

#define STDERR_WARN

#include <comperr.h>

#include <utility>

void Parser::iassert(bool cond, std::string what, ...) {
    va_list args;
    va_start(args, what);
    vcomperr(cond, what.c_str(), false, filename.c_str(), lexer.where().l, lexer.where().p, args);
    va_end(args);
}

void Parser::expect(TokenType raw) {
    iassert(lexer.peek().id == raw, "Unexpected '%s'.", lexer.peek().raw.c_str());
    lexer.consume();
}

void Parser::expect(const std::string & raw) {
    iassert(lexer.peek().raw == raw, "Expected '%s' found '%s'.", raw.c_str(), lexer.peek().raw.c_str());
    lexer.consume();
}

ExprType Parser::get_precedence() {
    if (infix_parslets.count(lexer.peek().id) > 0)
        return infix_parslets[lexer.peek().id]->get_type();
    return static_cast<ExprType>(0);
}

Parser::Parser(std::string code, std::string fn) :
        lexer(std::move(code)),
        filename(std::move(fn)) {
    // Trivial expressions
    prefix_parslets.emplace(NAME, new NameParselet());
    prefix_parslets.emplace(INTEGER, new IntParselet());
    prefix_parslets.emplace(REAL, new RealParselet());

    // Prefix expressions
    prefix_parslets.emplace(PLUS, new PosParselet());
    prefix_parslets.emplace(MINUS, new NegParselet());

    // Binary expressions
    infix_parslets.emplace(PLUS, new AddParselet());
    infix_parslets.emplace(MINUS, new SubParselet());
    infix_parslets.emplace(ASTERISK, new MulParselet());
    infix_parslets.emplace(SLASH, new DivParselet());
}

Parser::~Parser() {
    endfile();
}

Expression * Parser::parse_expression(int precedence) {
    Token token = lexer.consume();
    iassert(prefix_parslets.count(token.id) > 0, "Unkown token '%s'", token.raw.c_str());
    PrefixParselet * prefix = prefix_parslets[token.id];

    Expression * left = prefix->parse(this, token);

    while (precedence < get_precedence()) {
        token = lexer.consume();

        InfixParselet * infix = infix_parslets[token.id];
        left = infix->parse(this, left, token);
    }

    return left;
}

Statement * Parser::parse_statement() {
    TokenType type = lexer.peek().id;
    if (type == FUNC) {

    } else if (type == RETURN) {
        return new ReturnStatement(parse_expression());
    } else if (type == IF) {
        lexer.consume();
        expect(PAREN_OPEN);
        auto * res = new IfStatement(parse_expression());
        expect(PAREN_CLOSE);
        return res;
    }
    return nullptr;
}

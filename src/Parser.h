// Seno (c) Nikolas Wipper 2020

#ifndef SENO_PARSER_H
#define SENO_PARSER_H

#include <map>

#include "Lexer.h"
#include "expressions/Expression.h"
#include "expressions/Statements.h"

class PrefixParselet;
class InfixParselet;

class Parser {
    Lexer lexer;
    std::string filename;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    void iassert(bool cond, std::string what = "", ...);
    void expect(TokenType raw);
    void expect(const std::string & raw);

    ExprType get_precedence();

public:
    explicit Parser(std::string code, std::string fn = "<undefined>");
    ~Parser();

    Expression * parse_expression(int precedence = 0);
    Statement * parse_statement();
};


#endif //SENO_PARSER_H

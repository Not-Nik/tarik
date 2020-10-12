// Seno (c) Nikolas Wipper 2020

#ifndef SENO_PARSER_H
#define SENO_PARSER_H

#include <map>

#include "Lexer.h"
#include "expressions/Expression.h"

class PrefixParselet;
class InfixParselet;

class Parser {
    Lexer lexer;
    std::string filename;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    void iassert(bool cond, std::string what = "", ...);

    ExprType get_precedence();

public:
    explicit Parser(std::string code, std::string fn = "<undefined>");
    ~Parser();

    Expression * parse_expression(int precedence = 0);
};


#endif //SENO_PARSER_H

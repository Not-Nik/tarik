// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_PARSER_H
#define TARIK_PARSER_H

#include <map>
#include <vector>

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

    ExprType get_precedence();
    std::vector<Statement *> block();

public:
    explicit Parser(std::string code, std::string fn = "<undefined>");
    ~Parser();

    void expect(TokenType raw);
    void expect(const std::string & raw);

    Expression * parse_expression(int precedence = 0);
    Statement * parse_statement();
};


#endif //TARIK_PARSER_H

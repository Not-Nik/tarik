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
    bool has_variables = true;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    std::vector<VariableStatement *> variables;
    std::vector<int> variable_pop_stack;

    std::vector<StructStatement *> structures;
    std::vector<FuncStatement *> functions;

    Precedence get_precedence();

    std::vector<Statement *> block();

    Type type();

    Type type(Token starter);

public:
    explicit Parser(std::string code, std::string fn = "<undefined>");

    ~Parser();

    Parser * variable_less();

    bool iassert(bool cond, std::string what = "", ...);

    Token expect(TokenType raw);

    bool check_expect(TokenType raw);

    bool is_peek(TokenType raw);

    VariableStatement * require_var(const std::string & name);

    VariableStatement * register_var(VariableStatement * var);

    Expression * parse_expression(int precedence = 0);

    Statement * parse_statement(bool top_level = true);
};


#endif //TARIK_PARSER_H

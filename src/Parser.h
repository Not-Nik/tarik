// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_PARSER_H_
#define TARIK_PARSER_H_

#include <map>
#include <vector>

#include "Lexer.h"
#include "expressions/Expression.h"
#include "expressions/Statements.h"

class PrefixParselet;

class InfixParselet;

class Parser {
    friend class CallParselet;

    Lexer lexer;
    std::string filename;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    std::vector<VariableStatement *> variables;
    std::vector<int> variable_pop_stack;

    std::vector<StructStatement *> structures;
    std::vector<FuncStatement *> functions;

    Precedence get_precedence();

    std::vector<Statement *> block();
    Statement *scope();

    Type type();

public:
    explicit Parser(std::istream *code, std::string fn = "<undefined>");

    ~Parser();

    bool iassert(bool cond, std::string what, ...);
    bool iassert(bool cond, LexerPos pos, std::string what, ...);

    Token expect(TokenType raw);

    bool check_expect(TokenType raw);

    bool is_peek(TokenType raw);

    VariableStatement *require_var(const std::string &name);

    VariableStatement *register_var(VariableStatement *var);

    FuncStatement *require_func(const std::string &name);

    FuncStatement *register_func(FuncStatement *func);

    Expression *parse_expression(int precedence = 0);

    Statement *parse_statement(bool top_level = true);

    static int error_count();
};

#endif //TARIK_PARSER_H_

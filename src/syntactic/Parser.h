// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_SYNTACTIC_PARSER_H_
#define TARIK_SRC_SYNTACTIC_PARSER_H_

#include <map>
#include <vector>

#include "lexical/Lexer.h"
#include "syntactic/expressions/Expression.h"
#include "syntactic/expressions/Statements.h"

class PrefixParselet;

class InfixParselet;

class Parser {
    friend class CallParselet;

    Lexer lexer;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    std::vector<VariableStatement *> variables;

    std::vector<StructStatement *> structures;
    std::vector<FuncStatement *> functions;

    Precedence get_precedence();

    std::vector<Statement *> block();

    Type type();

    void init_parslets();

public:
    explicit Parser(std::istream *code);
    explicit Parser(const std::filesystem::path &f);

    ~Parser();

    bool iassert(bool cond, std::string what, ...);
    void warn(std::string what, ...);

    LexerPos where();

    Token expect(TokenType raw);

    bool check_expect(TokenType raw);

    bool is_peek(TokenType raw);

    VariableStatement *register_var(VariableStatement *var);
    FuncStatement *register_func(FuncStatement *func);
    StructStatement *register_struct(StructStatement *struct_);

    Expression *parse_expression(int precedence = 0);

    Statement *parse_statement();

    static int error_count();
};

#endif //TARIK_SRC_SYNTACTIC_PARSER_H_

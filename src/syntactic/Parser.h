// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_SYNTACTIC_PARSER_H_
#define TARIK_SRC_SYNTACTIC_PARSER_H_

#include <map>
#include <vector>
#include <filesystem>

#include "lexical/Lexer.h"
#include "syntactic/expressions/Expression.h"
#include "syntactic/expressions/Statements.h"

class PrefixParselet;

class InfixParselet;

class Parser {
    friend class CallParselet;

    static std::vector<std::filesystem::path> imported;

    Lexer lexer;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    std::vector<StructStatement *> structures;

    std::vector<std::filesystem::path> search_paths;

    bool dry_parsing;

    Precedence get_precedence();

    std::vector<Statement *> block();

    Type type();

    void init_parslets();

public:
    explicit Parser(std::istream *code, std::vector<std::filesystem::path> paths = {}, bool dry = false);
    explicit Parser(const std::filesystem::path &f, std::vector<std::filesystem::path> paths = {}, bool dry = false);

    ~Parser();

    bool iassert(bool cond, std::string what, ...);
    void warn(std::string what, ...);

    LexerPos where();

    Token expect(TokenType raw);
    bool check_expect(TokenType raw);
    bool is_peek(TokenType raw);

    std::filesystem::path find_import();

    StructStatement *register_struct(StructStatement *struct_);
    [[nodiscard]] bool has_struct_with_name(const std::string &name);

    Expression *parse_expression(int precedence = 0);

    Statement *parse_statement();
};

#endif //TARIK_SRC_SYNTACTIC_PARSER_H_

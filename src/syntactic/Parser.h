// tarik (c) Nikolas Wipper 2020-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SYNTACTIC_PARSER_H_
#define TARIK_SRC_SYNTACTIC_PARSER_H_

#include <map>
#include <vector>
#include <optional>
#include <filesystem>

#include "error/Bucket.h"
#include "error/Error.h"
#include "lexical/Lexer.h"
#include "syntactic/ast/Expression.h"
#include "syntactic/ast/Statements.h"

class PrefixParselet;

class InfixParselet;

class Parser {
    static std::vector<std::filesystem::path> imported;

    Lexer lexer;
    Bucket *bucket;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    std::vector<std::filesystem::path> search_paths;

    ast::Precedence get_precedence();

    std::vector<ast::Statement *> block();

    std::optional<Type> type();

    void init_parslets();

public:
    explicit Parser(std::istream *code, Bucket *bucket, std::vector<std::filesystem::path> paths = {});
    explicit Parser(const std::filesystem::path &f, Bucket *bucket, std::vector<std::filesystem::path> paths = {});

    ~Parser();

    Token expect(TokenType raw);
    bool check_expect(TokenType raw);
    bool is_peek(TokenType raw);

    std::filesystem::path find_import();

    ast::Expression *parse_expression(int precedence = 0);

    ast::Statement *parse_statement();
};

#endif //TARIK_SRC_SYNTACTIC_PARSER_H_

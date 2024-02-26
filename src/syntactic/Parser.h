// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SYNTACTIC_PARSER_H_
#define TARIK_SRC_SYNTACTIC_PARSER_H_

#include <map>
#include <vector>
#include <optional>
#include <filesystem>

#include "error/Error.h"
#include "lexical/Lexer.h"
#include "syntactic/expressions/Statements.h"

class PrefixParselet;

class InfixParselet;

class Parser {
    friend class CallParselet;

    static std::vector<std::filesystem::path> imported;

    Lexer lexer;

    std::map<TokenType, PrefixParselet *> prefix_parslets;
    std::map<TokenType, InfixParselet *> infix_parslets;

    std::vector<std::filesystem::path> search_paths;

    Precedence get_precedence();

    std::vector<Statement *> block();

    std::optional<Type> type();

    void init_parslets();

public:
    explicit Parser(std::istream *code, std::vector<std::filesystem::path> paths = {});
    explicit Parser(const std::filesystem::path &f, std::vector<std::filesystem::path> paths = {});

    ~Parser();

    template <class... Args>
    bool iassert(bool cond, std::format_string<Args...> what, Args &&... args) {
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        viassert(cond, where().as_zero_range(), str);
        if (!cond)
            lexer.read_until({';', '}'});
        return cond;
    }

    LexerPos where();

    Token expect(TokenType raw);
    bool check_expect(TokenType raw);
    bool is_peek(TokenType raw);

    std::filesystem::path find_import();

    Expression *parse_expression(int precedence = 0);

    Statement *parse_statement();
};

#endif //TARIK_SRC_SYNTACTIC_PARSER_H_

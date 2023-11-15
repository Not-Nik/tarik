// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_LEXICAL_LEXER_H_
#define TARIK_SRC_LEXICAL_LEXER_H_

#include "Token.h"

#include <vector>
#include <istream>
#include <filesystem>

class Lexer {
    std::istream *stream;
    bool allocated = false;

    LexerPos pos {1, 0, ""};

    static bool operator_startswith(char c);

    static bool operator_startswith(std::string c);

    char read_stream();
    char peek_stream();
    void unget_stream();

public:
    explicit Lexer(std::istream *s);
    explicit Lexer(const std::filesystem::path &f);
    ~Lexer();

    Token peek(int dist = 0);

    Token consume();

    LexerPos where();

    void read_until(std::vector<char> d);
};

#endif //TARIK_SRC_LEXICAL_LEXER_H_

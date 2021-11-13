// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_LEXICAL_LEXER_H_
#define TARIK_SRC_LEXICAL_LEXER_H_

#include "Token.h"

#include <istream>
#include <filesystem>

class Lexer {
    std::istream *stream;
    bool allocated = false;

    LexerPos pos{1, 0, ""};

    static bool operator_startswith(char c);

    static bool operator_startswith(std::string c);

    char read_stream();
    char peek_stream();
    void unget_stream();

public:
    explicit Lexer(std::istream *s);
    explicit Lexer(const std::filesystem::path& f);
    ~Lexer();

    Token peek(int dist = 0);

    Token consume();

    LexerPos where();
};

#endif //TARIK_SRC_LEXICAL_LEXER_H_

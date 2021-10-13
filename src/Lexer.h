// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_LEXER_H_
#define TARIK_LEXER_H_

#include "Token.h"

#include <istream>

struct LexerPos {
    int l, p;
};

class Lexer {
    std::istream *stream;

    LexerPos pos{0, 0};

    static bool operator_startswith(char c);

    static bool operator_startswith(std::string c);

    char read_stream();
    char peek_stream();

public:
    explicit Lexer(std::istream *s);

    Token peek(int dist = 0);

    Token consume();

    LexerPos where();
};

#endif //TARIK_LEXER_H_

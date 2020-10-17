// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_LEXER_H
#define TARIK_LEXER_H

#include "Token.h"

#include <sstream>

class Lexer {
    std::string code;

    struct {
        int l, p;
    } pos {0, 0};

    static bool operator_startswith(char c);

    static bool operator_startswith(std::string c);

public:
    explicit Lexer(std::string c);

    Token peek(int dist = 0);

    Token consume();

    typeof(Lexer::pos) where();
};

#endif //TARIK_LEXER_H

// Matrix (c) Nikolas Wipper 2020

#ifndef SENO_LEXER_H
#define SENO_LEXER_H

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

    Token peek();

    Token consume();

    typeof(Lexer::pos) where();
};

#endif //SENO_LEXER_H

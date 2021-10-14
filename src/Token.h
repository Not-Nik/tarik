// Matrix (c) Nikolas Wipper 2020

#ifndef TARIK_TOKEN_H_
#define TARIK_TOKEN_H_

#include <map>
#include <string>
#include <utility>

enum TokenType {
    END,
    INTEGER,
    REAL,
    STRING,
    NAME,
    PLUS,
    MINUS,
    ASTERISK,
    SLASH,
    PAREN_OPEN,
    PAREN_CLOSE,
    CURLY_OPEN,
    CURLY_CLOSE,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    COMMA,
    SEMICOLON,
    COLON,
    EQUAL,
    ARROW,
    DOUBLE_EQUAL,
    NOT_EQUAL,
    SMALLER_EQUAL,
    GREATER_EQUAL,
    SMALLER,
    GREATER,
    NOT,
    // Keywords
    FUNC,
    RETURN,
    IF,
    ELSE,
    WHILE,
    BREAK,
    CONTINUE,
    STRUCT,
    TYPE,
    USER_TYPE
};

static std::map<std::string, TokenType> operators = {
        {"+",  PLUS},
        {"-",  MINUS},
        {"*",  ASTERISK},
        {"/",  SLASH},
        {"(",  PAREN_OPEN},
        {")",  PAREN_CLOSE},
        {"{",  CURLY_OPEN},
        {"}",  CURLY_CLOSE},
        {"[",  BRACKET_OPEN},
        {"]",  BRACKET_CLOSE},
        {",",  COMMA},
        {";",  SEMICOLON},
        {":",  COLON},
        {"=",  EQUAL},
        {"->", ARROW},
        {"==", DOUBLE_EQUAL},
        {"!=", NOT_EQUAL},
        {"<=", SMALLER_EQUAL},
        {">=", GREATER_EQUAL},
        {"<",  SMALLER},
        {">",  GREATER},
        {"!",  NOT}};

static std::map<std::string, TokenType> keywords = {
        {"fn",       FUNC},
        {"return",   RETURN},
        {"if",       IF},
        {"else",     ELSE},
        {"while",    WHILE},
        {"break",    BREAK},
        {"continue", CONTINUE},
        {"struct",   STRUCT},
        {"i8",       TYPE},
        {"i16",      TYPE},
        {"i32",      TYPE},
        {"i64",      TYPE},
        {"u8",       TYPE},
        {"u16",      TYPE},
        {"u32",      TYPE},
        {"u64",      TYPE},
        {"f32",      TYPE},
        {"f64",      TYPE},
};

inline std::string to_string(const TokenType &tt) {
    for (auto op : operators) if (op.second == tt) return op.first;
    for (auto key : keywords) if (key.second == tt) return key.first;
    return "";
}

class Token {
public:
    explicit Token(TokenType id, std::string s)
        : id(id), raw(std::move(s)) {
    }

    TokenType id;
    std::string raw;
};

#endif //TARIK_TOKEN_H_

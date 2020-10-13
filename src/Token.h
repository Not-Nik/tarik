// Matrix (c) Nikolas Wipper 2020

#ifndef TARIK_TOKEN_H
#define TARIK_TOKEN_H

#include <map>
#include <string>
#include <utility>

enum TokenType {
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
    FOR,
    BREAK,
    CONTINUE,
    STRUCT,
    // Types
    INT_8,
    INT_16,
    INT_32,
    INT_64,
    UINT_8,
    UINT_16,
    UINT_32,
    UINT_64,
    FLOAT_32,
    FLOAT_64
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
        {"for",      FOR},
        {"break",    BREAK},
        {"continue", CONTINUE},
        {"struct",   STRUCT},
        {"i8",       INT_8},
        {"i16",      INT_16},
        {"i32",      INT_32},
        {"i64",      INT_64},
        {"u8",       UINT_8},
        {"u16",      UINT_16},
        {"u32",      UINT_32},
        {"u64",      UINT_64},
        {"f32",      FLOAT_32},
        {"f64",      FLOAT_64},
};

class Token {
public:
    explicit Token(TokenType id, std::string s) : raw(std::move(s)), id(id) {
    }

    TokenType id;
    std::string raw;
};

#endif //TARIK_TOKEN_H

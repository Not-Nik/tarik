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
        {"for",      FOR},
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

class Token {
public:
    explicit Token(TokenType id, std::string s) : raw(std::move(s)), id(id) {
    }

    TokenType id;
    std::string raw;
};

#endif //TARIK_TOKEN_H

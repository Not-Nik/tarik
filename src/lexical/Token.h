// Matrix (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_LEXICAL_TOKEN_H_
#define TARIK_SRC_LEXICAL_TOKEN_H_

#include <map>
#include <string>
#include <utility>
#include <filesystem>

enum TokenType {
    END,
    INTEGER,
    REAL,
    STRING,
    NAME,
    PLUS,
    MINUS,
    ASTERISK,
    AMPERSAND,
    SLASH,
    PAREN_OPEN,
    PAREN_CLOSE,
    CURLY_OPEN,
    CURLY_CLOSE,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    PERIOD,
    TRIPLE_PERIOD,
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
    DOLLAR,
    // Keywords
    FUNC,
    RETURN,
    IF,
    ELSE,
    TRUE,
    FALSE,
    WHILE,
    BREAK,
    CONTINUE,
    STRUCT,
    NULL_,
    IMPORT,
    TYPE,
    USER_TYPE
};

inline std::map<std::string, TokenType> operators = {
        {"+",   PLUS},
        {"-",   MINUS},
        {"*",   ASTERISK},
        {"&",   AMPERSAND},
        {"/",   SLASH},
        {"(",   PAREN_OPEN},
        {")",   PAREN_CLOSE},
        {"{",   CURLY_OPEN},
        {"}",   CURLY_CLOSE},
        {"[",   BRACKET_OPEN},
        {"]",   BRACKET_CLOSE},
        {".",   PERIOD},
        {"...", TRIPLE_PERIOD},
        {",",   COMMA},
        {";",   SEMICOLON},
        {":",   COLON},
        {"=",   EQUAL},
        {"->",  ARROW},
        {"==",  DOUBLE_EQUAL},
        {"!=",  NOT_EQUAL},
        {"<=",  SMALLER_EQUAL},
        {">=",  GREATER_EQUAL},
        {"<",   SMALLER},
        {">",   GREATER},
        {"!",   NOT},
        {"$",   DOLLAR}};

inline std::map<std::string, TokenType> keywords = {
        {"fn",       FUNC},
        {"return",   RETURN},
        {"if",       IF},
        {"else",     ELSE},
        {"true",     TRUE},
        {"false",    FALSE},
        {"while",    WHILE},
        {"break",    BREAK},
        {"continue", CONTINUE},
        {"struct",   STRUCT},
        {"null",     NULL_},
        {"import",   IMPORT},
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
        {"bool",     TYPE}
};

inline std::string to_string(const TokenType &tt) {
    for (const auto &op: operators) if (op.second == tt) return "'" + op.first + "'";
    for (const auto &key: keywords) if (key.second == tt) return "'" + key.first + "'";
    if (tt == NAME) return "name";
    if (tt == STRING) return "string";
    if (tt == INTEGER || tt == REAL) return "number";
    return "";
}

struct LexerPos {
    int l, p;
    std::filesystem::path filename;

    LexerPos &operator--() {
        if (p > 0) p--;
        else {
            // ugh, ill accept the inaccuracy for now
            // todo: make this accurate
            p = 0;
            l--;
        }
        return *this;
    }
};

class Token {
public:
    explicit Token(TokenType id, std::string s, LexerPos lp)
        : id(id), raw(std::move(s)), where(std::move(lp)) {
    }

    TokenType id;
    std::string raw;
    LexerPos where;
};

#endif //TARIK_SRC_LEXICAL_TOKEN_H_

// Matrix (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_LEXICAL_TOKEN_H_
#define TARIK_SRC_LEXICAL_TOKEN_H_

#include <map>
#include <string>
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
    DOUBLE_COLON,
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
    {"+", PLUS},
    {"-", MINUS},
    {"*", ASTERISK},
    {"&", AMPERSAND},
    {"/", SLASH},
    {"(", PAREN_OPEN},
    {")", PAREN_CLOSE},
    {"{", CURLY_OPEN},
    {"}", CURLY_CLOSE},
    {"[", BRACKET_OPEN},
    {"]", BRACKET_CLOSE},
    {".", PERIOD},
    {"...", TRIPLE_PERIOD},
    {",", COMMA},
    {";", SEMICOLON},
    {":", COLON},
    {"::", DOUBLE_COLON},
    {"=", EQUAL},
    {"->", ARROW},
    {"==", DOUBLE_EQUAL},
    {"!=", NOT_EQUAL},
    {"<=", SMALLER_EQUAL},
    {">=", GREATER_EQUAL},
    {"<", SMALLER},
    {">", GREATER},
    {"!", NOT},
    {"$", DOLLAR}
};

inline std::map<std::string, TokenType> keywords = {
    {"fn", FUNC},
    {"return", RETURN},
    {"if", IF},
    {"else", ELSE},
    {"true", TRUE},
    {"false", FALSE},
    {"while", WHILE},
    {"break", BREAK},
    {"continue", CONTINUE},
    {"struct", STRUCT},
    {"null", NULL_},
    {"import", IMPORT},
    {"i8", TYPE},
    {"i16", TYPE},
    {"i32", TYPE},
    {"i64", TYPE},
    {"u8", TYPE},
    {"u16", TYPE},
    {"u32", TYPE},
    {"u64", TYPE},
    {"f32", TYPE},
    {"f64", TYPE},
    {"bool", TYPE}
};

std::string to_string(const TokenType &tt);

struct LexerRange;

struct LexerPos {
    int l = 0, p = 0;
    std::filesystem::path filename;

    LexerRange as_zero_range() const;

    LexerPos &operator--();
    // range from pos1 to pos2
    LexerRange operator-(LexerPos other);
};

struct LexerRange : LexerPos {
    int length = 0;

    LexerRange operator+(LexerRange other);
};

class Token {
public:
    explicit Token(TokenType id, std::string s, LexerRange lp)
        : id(id),
          raw(std::move(s)),
          origin(std::move(lp)) {
    }

    static Token name(std::string s, LexerRange lp = {}) {
        return Token(NAME, s, lp);
    }

    TokenType id;
    std::string raw;
    LexerRange origin;
};

#endif //TARIK_SRC_LEXICAL_TOKEN_H_

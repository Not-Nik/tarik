// tarik (c) Nikolas Wipper 2024

#include "Token.h"

std::string to_string(const TokenType &tt) {
    for (const auto &op : operators)
        if (op.second == tt)
            return "'" + op.first + "'";
    for (const auto &key : keywords)
        if (key.second == tt)
            return "'" + key.first + "'";
    if (tt == NAME)
        return "name";
    if (tt == STRING)
        return "string";
    if (tt == INTEGER || tt == REAL)
        return "number";
    return "";
}

LexerRange LexerPos::as_zero_range() const {
    return LexerRange { *this, 0 };
}

LexerPos &LexerPos::operator--() {
    if (p > 1)
        p--;
    else {
        // ugh, ill accept the inaccuracy for now
        // todo: make this accurate
        p = 1;
        l--;
    }
    return *this;
}

LexerRange LexerPos::operator-(LexerPos other) {
    if (this->filename != other.filename || this->l != other.l) {
        return {};
    }

    return LexerRange { *this, other.p - this->p };
}


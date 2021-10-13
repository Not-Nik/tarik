// tarik (c) Nikolas Wipper 2020

#include "Lexer.h"

#include <utility>
#include <algorithm>

Lexer::Lexer(std::istream *s)
    : stream(s) {
}

bool Lexer::operator_startswith(char c) {
    return std::any_of(operators.begin(), operators.end(), [c](auto op) { return op.first[0] == c; });
}

bool Lexer::operator_startswith(std::string c) {
    return std::any_of(operators.begin(), operators.end(), [c](auto op) { return op.first.find(c) != std::string::npos; });
}

char Lexer::read_stream() {
    char c = (char) stream->get();
    if (c == '\n') {
        pos.l++;
        pos.p = 0;
    } else pos.p++;
    return c;
}

char Lexer::peek_stream() {
    return (char) stream->peek();
}

Token Lexer::peek(int dist) {
    auto old_pos = stream->tellg();
    Token t = consume();
    for (int i = 0; i < dist; i++) {
        t = consume();
    }
    stream->clear();
    stream->seekg(old_pos);
    return t;
}

Token Lexer::consume() {
    std::string tok;
    bool num = false, real = false, op = false, string = false;

    while (!stream->eof()) {
        char c = read_stream();
        if (c < 0) break;

        if (c == '"') {
            if (!tok.empty()) break;
            tok.push_back(c);
            c = read_stream();
            for (; c != '\"'; c = read_stream()) {
                tok.push_back(c);
            }
            tok.push_back(c);
            string = true;
            break;
        }

        if (c == '#') {
            for (; c != '\n'; c = read_stream()) {}
            continue;
        }

        // Basically stop the token if we have an operator that is right after another token i.e. `peter*`
        if (!tok.empty() && // If we are in a token
            operator_startswith(c) && // And the current char does start an operator
            !operator_startswith(tok + (char) c)) { // But the token and that char do not start an operator
            stream->unget();
            break; // Break
        }

        if (operator_startswith(c) && c != '.') {
            if (tok.empty()) {
                op = true;
                tok.push_back(c);
            } else if (op) {
                tok.push_back(c);
            } else {
                stream->unget();
                break;
            }
            continue;
        }

        if (op || isspace(c)) {
            stream->unget();
            break;
        }

        if (isnumber(c)) {
            if (num || tok.empty()) {
                num = true;
            }
            tok.push_back(c);
        } else if (c == '.') {
            char cs = peek_stream();
            if (tok.empty()) {
                if (isnumber(cs)) {
                    tok = {'0', '.', cs};
                    num = true;
                    real = true;
                    read_stream();
                } else {
                    tok = ".";
                    break;
                }
            } else {
                if (num && isnumber(cs) && !real) {
                    tok.append({'.', cs});
                    real = true;
                    read_stream();
                } else {
                    stream->unget();
                    break;
                }
            }
        } else {
            if (!num) {
                tok.push_back(c);
            } else {
                // No scientific notation for now
                stream->unget();
                break;
            }
        }
    }

    while (isspace(peek_stream())) {
        read_stream();
    }

    TokenType type;
    if (op && operators.count(tok)) {
        type = operators[tok];
    } else if (keywords.count(tok)) {
        type = keywords[tok];
    } else if (real) {
        type = REAL;
    } else if (num) {
        type = INTEGER;
    } else if (string) {
        type = STRING;
    } else {
        type = NAME;
    }
    return Token(type, tok);
}

LexerPos Lexer::where() {
    return this->pos;
}

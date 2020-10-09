// Matrix (c) Nikolas Wipper 2020

#include "Lexer.h"

#include <utility>

Lexer::Lexer(std::string c) : code(std::move(c)) {
}

bool Lexer::operator_startswith(char c) {
    return std::any_of(operators.begin(), operators.end(), [c](auto op) { return op.first[0] == c; });
}

bool Lexer::operator_startswith(std::string c) {
    return std::any_of(operators.begin(), operators.end(), [c](auto op) { return op.first.find(c) != std::string::npos; });
}

Token Lexer::peek() {
    std::string tok;
    bool num = false, real = false, op = false, string = false;
    for (int i = 0; i < code.size(); i++) {
        char c = code[i];

        if (c == '"') {
            if (tok.empty()) {
                string = true;
                tok.push_back(c);
                continue;
            } else {
                if (string) {
                    tok.push_back(c);
                }
                break;
            }
        }

        if (string) {
            tok.push_back(c);
            continue;
        }

        if (isblank(c)) {
            if (tok.empty()) continue;
            break;
        }

        if (!tok.empty() && operator_startswith(c) && !operator_startswith(tok + c))
            break;

        if (!isalnum(c) && c != '.') {
            if (tok.empty()) {
                op = true;
                tok.push_back(c);
            } else if (op) {
                tok.push_back(c);
            } else {
                break;
            }
            continue;
        }

        if (op) break;

        if (isalpha(c)) {
            if (!num) {
                tok.push_back(c);
            } else {
                // No scientific notation for now
                break;
            }
        } else if (isnumber(c)) {
            if (num || tok.empty()) {
                num = true;
            }
            tok.push_back(c);
        } else if (c == '.') {
            if (tok.empty()) {
                char cs = code[i + 1];
                if (isnumber(cs)) {
                    tok = {'0', '.', cs};
                    num = true;
                    real = true;
                    i++;
                } else {
                    tok = ".";
                    break;
                }
            } else {
                char cs = code[i + 1];
                if (num && isnumber(cs) && !real) {
                    tok.append({'.', cs});
                    real = true;
                    i++;
                } else {
                    break;
                }
            }
        }
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

Token Lexer::consume() {
    Token t = peek();
    code.erase(0, t.raw.size());
    while (isblank(code[0])) {
        code.erase(0, 1);
    }
    return t;
}

typeof(Lexer::pos) Lexer::where() {
    return this->pos;
}

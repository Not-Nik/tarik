// tarik (c) Nikolas Wipper 2020

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

Token Lexer::peek(int dist) {
    std::string tok;
    bool num = false, real = false, op = false, string = false;

    int d = 0;
    while (dist--) {
        d += peek(dist).raw.size();
        while (isblank(code[d])) {
            d++;
        }
    }

    for (int i = d; i < code.size(); i++) {
        char c = code[i];

        if (c == '"') {
            if (!tok.empty())
                break;
            tok.push_back(code[i++]);
            for (; code[i] != '\"'; i++) {
                tok.push_back(code[i]);
            }
            tok.push_back(code[i]);
            string = true;
            break;
        }

        if (c == '#') {
            for (; code[i] != '\n'; i++) { }
            continue;
        }

        // Basically stop the token if we have an operator that is right after another token i.e. `peter*`
        if (!tok.empty() && // If we are in a token
            operator_startswith(c) && // And the current char does start an operator
            !operator_startswith(tok + c)) // But the token and that char do not start an operator
            break; // Break

        if (operator_startswith(c) && c != '.') {
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

        if (op || isblank(c))
            break;

        if (isnumber(c)) {
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
        } else {
            if (!num) {
                tok.push_back(c);
            } else {
                // No scientific notation for now
                break;
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
    pos.p += t.raw.size();
    while (isblank(code[0])) {
        if (code[0] == '\n')
            pos = {.l = pos.l+1, .p=0};
        else
            pos.p++;
        code.erase(0, 1);
    }
    return t;
}

typeof(Lexer::pos) Lexer::where() {
    return this->pos;
}

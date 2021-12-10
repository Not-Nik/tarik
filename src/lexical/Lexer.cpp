// tarik (c) Nikolas Wipper 2020

#include "Lexer.h"

#include <utility>
#include <fstream>
#include <algorithm>

Lexer::Lexer(std::istream *s)
    : stream(s) {
}

Lexer::Lexer(const std::filesystem::path &f)
    : Lexer(new std::ifstream(f)) {
    allocated = true;
    pos.filename = f;
}

Lexer::~Lexer() {
    if (allocated) delete stream;
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

void Lexer::unget_stream() {
    --pos;
    stream->unget();
}

Token Lexer::peek(int dist) {
    auto old_pos = stream->tellg();
    auto old_int_pos = pos;
    Token t = consume();
    for (int i = 0; i < dist; i++) {
        t = consume();
    }
    stream->clear();
    stream->seekg(old_pos);
    pos = old_int_pos;
    return t;
}

std::string post_process_string(std::string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\') {
            s.erase(1);
            i++;
            switch (s[i]) {
                case '?':
                    s[i] = '\?';
                    break;
                case '\\':
                    break;
                case 'a':
                    s[i] = '\a';
                    break;
                case 'b':
                    s[i] = '\b';
                    break;
                case 'f':
                    s[i] = '\f';
                    break;
                case 'n':
                    s[i] = '\n';
                    break;
                case 'r':
                    s[i] = '\r';
                    break;
                case 't':
                    s[i] = '\t';
                    break;
                case 'v':
                    s[i] = '\v';
                    break;
            }
        }
    }

    return s;
}

Token Lexer::consume() {
    std::string tok;
    bool num = false, real = false, op = false, string = false;

    while (!stream->eof()) {
        char c = read_stream();
        if (c < 0) break;

        if (c == '"') {
            if (!tok.empty()) {
                unget_stream();
                break;
            }
            c = read_stream();
            for (; c != '\"'; c = read_stream()) {
                if (c == '\\') {
                    c = read_stream();
                    switch (c) {
                        case '\'':
                            c = '\'';
                            break;
                        case '"':
                            c = '"';
                            break;
                        case '?':
                            c = '\?';
                            break;
                        case '\\':
                            c = '\\';
                            break;
                        case 'a':
                            c = '\a';
                            break;
                        case 'b':
                            c = '\b';
                            break;
                        case 'f':
                            c = '\f';
                            break;
                        case 'n':
                            c = '\n';
                            break;
                        case 'r':
                            c = '\r';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        case 'v':
                            c = '\v';
                            break;
                        default:
                            continue;
                    }
                }
                tok.push_back(c);
            }
            string = true;
            break;
        }

        if (c == '#') {
            while (c != '\n') c = read_stream();
            while (isspace(peek_stream())) read_stream();
            if (tok.empty()) continue;
            else break;
        }

        // Basically stop the token if we have an operator that is right after another token i.e. `peter*`
        if (!tok.empty() && // If we are in a token
            operator_startswith(c) && // And the current char starts an operator
            !operator_startswith(tok + (char) c) && // But the token and that char do not start an operator
            (c != '.')) { // And c isn't '.', because we need that for numbers
            unget_stream();
            break; // Break
        }

        // This is a bit hacky, but it works ig
        // TODO: make this clean
        if (operator_startswith(c) && (c != '.' || tok.empty() || tok == "." || tok == "..")) {
            if (tok.empty()) {
                op = true;
                tok.push_back(c);
            } else if (op) {
                tok.push_back(c);
            } else {
                unget_stream();
                break;
            }
            continue;
        }

        if (op || isspace(c)) {
            unget_stream();
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
                    unget_stream();
                    break;
                }
            }
        } else {
            if (!num) {
                tok.push_back(c);
            } else {
                // No scientific notation for now
                unget_stream();
                break;
            }
        }
    }

    auto actual_pos = pos;
    while (isspace(peek_stream())) read_stream();

    if (stream->eof() && tok.empty()) {
        return Token(END, "", actual_pos);
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
    return Token(type, tok, actual_pos);
}

LexerPos Lexer::where() {
    return this->pos;
}

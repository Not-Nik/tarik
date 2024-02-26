// tarik (c) Nikolas Wipper 2024

#ifndef TARIK_SRC_ERROR_ERROR_H_
#define TARIK_SRC_ERROR_ERROR_H_

#include <format>

#include "lexical/Token.h"

void verror(LexerRange pos, std::string what);
void vwarning(LexerRange pos, std::string what);
void vnote(LexerRange pos, std::string what);
bool viassert(bool cond, LexerRange pos, std::string what);

template <class... Args>
void error(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
    std::string str = std::vformat(what.get(), std::make_format_args(args...));
    verror(pos, str);
}

template <class... Args>
void warning(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
    std::string str = std::vformat(what.get(), std::make_format_args(args...));
    vwarning(pos, str);
}

template <class... Args>
void note(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
    std::string str = std::vformat(what.get(), std::make_format_args(args...));
    vnote(pos, str);
}

template <class... Args>
bool iassert(bool cond, LexerRange pos, std::format_string<Args...> what, Args &&... args) {
    std::string str = std::vformat(what.get(), std::make_format_args(args...));
    return viassert(cond, pos, str);
}

#endif //TARIK_SRC_ERROR_ERROR_H_

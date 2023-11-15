// tarik (c) Nikolas Wipper 2021-2023

#ifndef TARIK_SRC_UTIL_UTIL_H_
#define TARIK_SRC_UTIL_UTIL_H_

#include "lexical/Lexer.h"

#include <string>
#include <cstdarg>

void error(LexerPos pos, std::string what, ...);
void warning(LexerPos pos, std::string what, ...);
void note(LexerPos pos, std::string what, ...);

void error_v(LexerPos pos, std::string what, va_list args);
void warning_v(LexerPos pos, std::string what, va_list args);
void note_v(LexerPos pos, std::string what, va_list args);

bool iassert(bool cond, LexerPos pos, std::string what, ...);
bool iassert(bool cond, LexerPos pos, std::string what, va_list list);

#endif //TARIK_SRC_UTIL_UTIL_H_

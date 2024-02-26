// tarik (c) Nikolas Wipper 2021-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_UTIL_UTIL_H_
#define TARIK_SRC_UTIL_UTIL_H_

#include "lexical/Lexer.h"

#include <string>
#include <cstdarg>

void error(LexerRange pos, std::string what, ...);
void warning(LexerRange pos, std::string what, ...);
void note(LexerRange pos, std::string what, ...);

void error_v(LexerRange pos, std::string what, va_list args);
void warning_v(LexerRange pos, std::string what, va_list args);
void note_v(LexerRange pos, std::string what, va_list args);

bool iassert(bool cond, LexerRange pos, std::string what, ...);
bool iassert(bool cond, LexerRange pos, std::string what, va_list list);

#endif //TARIK_SRC_UTIL_UTIL_H_

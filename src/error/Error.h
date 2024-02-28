// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_ERROR_ERROR_H_
#define TARIK_SRC_ERROR_ERROR_H_

#include "lexical/Token.h"

enum class ErrorKind {
    ERROR,
    WARNING,
    NOTE
};

struct Error {
    ErrorKind kind;
    std::string message;
    LexerRange range;

    void print();
};

void verror(LexerRange pos, std::string what);
void vwarning(LexerRange pos, std::string what);
void vnote(LexerRange pos, std::string what);
bool viassert(bool cond, LexerRange pos, std::string what);

#endif //TARIK_SRC_ERROR_ERROR_H_

// tarik (c) Nikolas Wipper 2021-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Util.h"

#include <comperr.h>

void comp_msg(LexerRange pos, message_kind kind, std::string what, va_list args) {
    vcomperr(what.c_str(), kind, pos.filename.c_str(), pos.l, pos.p, pos.length, args);
}

void error(LexerRange pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    error_v(pos, what, args);
    va_end(args);
}

void warning(LexerRange pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    warning_v(pos, what, args);
    va_end(args);
}

void note(LexerRange pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    note_v(pos, what, args);
    va_end(args);
}

void error_v(LexerRange pos, std::string what, va_list args) {
    comp_msg(pos, ERROR, what, args);
}

void warning_v(LexerRange pos, std::string what, va_list args) {
    comp_msg(pos, WARNING, what, args);
}

void note_v(LexerRange pos, std::string what, va_list args) {
    comp_msg(pos, NOTE, what, args);
}

bool iassert(bool cond, LexerRange pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    iassert(cond, pos, what, args);
    va_end(args);
    return cond;
}

bool iassert(bool cond, LexerRange pos, std::string what, va_list list) {
    if (!cond)
        vcomperr(what.c_str(), ERROR, pos.filename.c_str(), pos.l, pos.p, pos.length, list);
    return cond;
}

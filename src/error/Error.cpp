// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Error.h"

#include <comperr.h>

void verror(LexerRange pos, std::string what) {
    comperr(what.c_str(), ERROR, pos.filename.c_str(), pos.l, pos.p, pos.length);
}

void vwarning(LexerRange pos, std::string what) {
    comperr(what.c_str(), WARNING, pos.filename.c_str(), pos.l, pos.p, pos.length);
}

void vnote(LexerRange pos, std::string what) {
    comperr(what.c_str(), NOTE, pos.filename.c_str(), pos.l, pos.p, pos.length);
}

bool viassert(bool cond, LexerRange pos, std::string what) {
    if (!cond)
        comperr(what.c_str(), ERROR, pos.filename.c_str(), pos.l, pos.p, pos.length);
    return cond;
}

// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Error.h"

#include <comperr.h>

#include "Bucket.h"

bool Error::assert(bool cond) {
    if (!cond) {
        bucket->errors.emplace(*this);
        bucket->error_count += kind == ErrorKind::ERROR ? 1 : 0;
    }

    kind = ErrorKind::STAGING;
    notes.clear();

    return cond;
}

void Error::print() const {
    if (kind == ErrorKind::ERROR)
        verror(origin, message);
    else if (kind == ErrorKind::WARNING)
        vwarning(origin, message);

    for (auto &note : notes)
        vnote(note.origin, note.message);
}


std::size_t std::hash<Note>::operator()(const Note &e) const noexcept {
    std::size_t seed = 0;
    seed += std::hash<std::string>{}(e.message);
    seed += std::hash<LexerRange>{}(e.origin);

    return seed;
}

std::size_t std::hash<Error>::operator()(const Error &e) const noexcept {
    std::size_t seed = 0;
    seed += std::hash<ErrorKind>{}(e.kind);
    seed += std::hash<std::string>{}(e.message);
    seed += std::hash<LexerRange>{}(e.origin);
    seed += std::hash<Bucket*>{}(e.bucket);

    for (const auto &note : e.notes) {
        seed += std::hash<Note>{}(note);
    }

    return seed;
}

void verror(const LexerRange &pos, const std::string &what) {
    comperr(what.c_str(), ERROR, pos.filename.c_str(), pos.l, pos.p, pos.length);
}

void vwarning(const LexerRange &pos, const std::string &what) {
    comperr(what.c_str(), WARNING, pos.filename.c_str(), pos.l, pos.p, pos.length);
}

void vnote(const LexerRange &pos, const std::string &what) {
    comperr(what.c_str(), NOTE, pos.filename.c_str(), pos.l, pos.p, pos.length);
}

bool viassert(bool cond, const LexerRange &pos, const std::string &what) {
    if (!cond)
        comperr(what.c_str(), ERROR, pos.filename.c_str(), pos.l, pos.p, pos.length);
    return cond;
}

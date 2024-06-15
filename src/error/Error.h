// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_ERROR_ERROR_H_
#define TARIK_SRC_ERROR_ERROR_H_

#include <vector>

#include "lexical/Token.h"

#undef assert

class Bucket;

enum class ErrorKind {
    STAGING,
    ERROR,
    WARNING,
};

struct Note {
    std::string message;
    LexerRange origin;

    bool operator==(const Note &) const = default;
};

struct Error {
    ErrorKind kind;
    std::string message;
    LexerRange origin;
    Bucket *bucket;

    std::vector<Note> notes = {};

    template <class... Args>
    Error *note(const LexerRange &pos, std::format_string<Args...> what, Args &&... args) {
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        notes.emplace_back(str, pos);
        return this;
    }

    bool assert(bool cond);
    void print() const;

    bool operator==(const Error &other) const = default;
};

template <>
struct std::hash<Note> {
    std::size_t operator()(const Note &s) const noexcept;
};

template <>
struct std::hash<Error> {
    std::size_t operator()(const Error &s) const noexcept;
};

void verror(const LexerRange& pos, const std::string &what);
void vwarning(const LexerRange & pos, const std::string & what);
void vnote(const LexerRange & pos, const std::string & what);
bool viassert(bool cond, const LexerRange & pos, const std::string & what);

#endif //TARIK_SRC_ERROR_ERROR_H_

// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_ERROR_BUCKET_H_
#define TARIK_SRC_ERROR_BUCKET_H_

#include "Error.h"

#include <format>
#include <vector>

class Bucket {
    std::vector<Error> errors;

    void add_error(LexerRange pos, std::string message);
    void add_warning(LexerRange pos, std::string message);
    void add_note(LexerRange pos, std::string message);

public:
    template <class... Args>
    void error(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        add_error(pos, str);
    }

    template <class... Args>
    void warning(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        add_warning(pos, str);
    }

    template <class... Args>
    void note(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        add_note(pos, str);
    }

    template <class... Args>
    bool iassert(bool cond, LexerRange pos, std::format_string<Args...> what, Args &&... args) {
        if (cond)
            return true;
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        add_error(pos, str);
        return false;
    }

    size_t error_count();
    std::vector<Error> finish();
    void print_errors() const;
};

#endif //TARIK_SRC_ERROR_BUCKET_H_

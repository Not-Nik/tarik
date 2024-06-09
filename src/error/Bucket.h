// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_ERROR_BUCKET_H_
#define TARIK_SRC_ERROR_BUCKET_H_

#include "Error.h"

#include <format>
#include <utility>
#include <vector>

class Bucket {
    friend struct Error;

    std::vector<Error> errors;
    size_t error_count = 0;

    Error staging_error;

    Error *clear_staging_error(LexerRange origin, std::string message, ErrorKind kind);

public:
    template <class... Args>
    Error *error(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        return clear_staging_error(std::move(pos), str, ErrorKind::ERROR);
    }

    template <class... Args>
    Error *warning(LexerRange pos, std::format_string<Args...> what, Args &&... args) {
        std::string str = std::vformat(what.get(), std::make_format_args(args...));
        return clear_staging_error(std::move(pos), str, ErrorKind::WARNING);
    }

    size_t get_error_count();
    std::vector<Error> get_errors();

    void print_errors();
};

#endif //TARIK_SRC_ERROR_BUCKET_H_

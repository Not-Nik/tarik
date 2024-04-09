// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Bucket.h"

void Bucket::add_error(LexerRange pos, std::string message) {
    errors.push_back(Error {
        ErrorKind::ERROR,
        message,
        pos
    });
    error_count++;
}

void Bucket::add_warning(LexerRange pos, std::string message) {
    errors.push_back(Error {
        ErrorKind::WARNING,
        message,
        pos
    });
}

void Bucket::add_note(LexerRange pos, std::string message) {
    errors.push_back(Error {
        ErrorKind::NOTE,
        message,
        pos
    });
}

size_t Bucket::get_error_count() {
    return error_count;
}

std::vector<Error> Bucket::finish() {
    return errors;
}

void Bucket::print_errors() const {
    for (auto error : errors) {
        switch (error.kind) {
            case ErrorKind::ERROR:
                verror(error.range, error.message);
                break;
            case ErrorKind::WARNING:
                vwarning(error.range, error.message);
                break;
            case ErrorKind::NOTE:
                vnote(error.range, error.message);
                break;
        }
    }
}

// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Bucket.h"

Error *Bucket::clear_staging_error(LexerRange origin, std::string message, ErrorKind kind) {
    if (staging_error.kind != ErrorKind::STAGING)
        errors.push_back(staging_error);

    return &(staging_error = Error { kind, message, std::move(origin), this });
}


size_t Bucket::get_error_count() const {
    return error_count;
}

std::vector<Error> Bucket::get_errors() {
    if (staging_error.kind != ErrorKind::STAGING)
        errors.push_back(staging_error);
    return errors;
}

void Bucket::print_errors() {
    if (staging_error.kind != ErrorKind::STAGING)
        errors.push_back(staging_error);

    for (const auto &error : errors) {
        error.print();
    }
}

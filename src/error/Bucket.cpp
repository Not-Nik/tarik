// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Bucket.h"

#include <algorithm>

#include <utility>

Error *Bucket::clear_staging_error(LexerRange origin, std::string message, ErrorKind kind) {
    if (staging_error.kind != ErrorKind::STAGING) {
        errors.push_back(staging_error);
        if (staging_error.kind == ErrorKind::ERROR)
            error_count++;
    }

    return &(staging_error = Error {kind, std::move(message), std::move(origin), this});
}

size_t Bucket::get_error_count() {
    clear_staging_error({}, {}, ErrorKind::STAGING);
    return error_count;
}

std::vector<Error> Bucket::get_errors() {
    clear_staging_error({}, {}, ErrorKind::STAGING);

    std::stable_sort(errors.begin(),
                     errors.end(),
                     [](const Error &e1, const Error &e2) {
                         return !(e1.origin > e2.origin);
                     });

    return errors;
}

void Bucket::print_errors() {
    for (const auto &error : get_errors()) {
        error.print();
    }
}

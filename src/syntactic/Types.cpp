// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Types.h"

#include <numeric>

#include "ast/Statements.h"
#include "utf/Utf.h"

std::string Type::str() const {
    std::string res = base();
    if (pointer_level != 0) {
        res.push_back(' ');
        for (int i = 0; i < pointer_level; i++)
            res += "*";
    }
    return res;
}

std::string Type::base() const {
    std::string res;
    if (type.index() == 0) {
        res = to_string(std::get<TypeSize>(type));
    } else {
        res = std::get<Path>(type).str();
    }
    return res;
}

std::string Type::func_name() const {
    std::u32string base_str = to_utf(base());
    std::u32string res;

    std::locale en_US_UTF_8("en_US.UTF-8");

    for (auto c : base_str) {
        // Todo: this does not work on Windows
        if (std::isupper((wchar_t)c, en_US_UTF_8)) {
            if (!res.empty())
                res.push_back('_');
            res.push_back(std::tolower((wchar_t)c, en_US_UTF_8));
        } else
            res.push_back(c);
    }

    return to_string(res);
}

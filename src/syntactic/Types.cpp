// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Types.h"

#include <numeric>

#include "ast/Statements.h"

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
    std::string base_str = base();
    std::string res;

    for (auto c : base_str) {
        if (isupper(c))
            res.insert(res.end(), {'_', (char) tolower(c)});
        else
            res.push_back(c);
    }

    return res;
}

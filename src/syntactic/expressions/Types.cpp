// tarik (c) Nikolas Wipper 2021-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Types.h"

#include "Statements.h"

std::string Type::str() const {
    std::string res;
    if (type.index() == 0) {
        res = to_string(std::get<TypeSize>(type));
    } else {
        res = std::get<std::string>(type);
    }
    if (pointer_level != 0) {
        res.push_back(' ');
        for (int i = 0; i < pointer_level; i++)
            res += "*";
    }
    return res;
}

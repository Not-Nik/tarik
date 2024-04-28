// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_MACRO_H_
#define TARIK_SRC_SEMANTIC_MACRO_H_

#include <vector>

#include "ast/Expression.h"

class Macro {
public:
    enum ArgumentType {
        TYPE,
        EXPRESSION,
        IDENTIFIER
    };

    std::vector<ArgumentType> arguments;

    virtual ast::Expression *apply(std::vector<ast::Expression> arguments) = 0;
};

#endif //TARIK_SRC_SEMANTIC_MACRO_H_

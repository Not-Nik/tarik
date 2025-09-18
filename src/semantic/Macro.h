// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_MACRO_H_
#define TARIK_SRC_SEMANTIC_MACRO_H_

#include <vector>

#include "ast/Expression.h"
#include "syntactic/ast/Statements.h"

class Analyser;

class Macro {
public:
    enum ArgumentType {
        TYPE,
        EXPRESSION,
        IDENTIFIER,
        REPEAT
    };

    std::vector<ArgumentType> arguments;

    virtual ast::Expression *apply(Analyser *analyser,
                                   ast::Expression *macro_call,
                                   std::vector<ast::Expression *> arguments) = 0;
};

class CastMacro : public Macro {
public:
    CastMacro();

    ast::Expression *apply(Analyser *analyser,
                           ast::Expression *macro_call,
                           std::vector<ast::Expression *> arguments) override;
};

template <bool VARIABLE_ARGS>
class ExternMacro : public Macro {
public:
    ExternMacro();

    ast::Expression *apply(Analyser *analyser,
                           ast::Expression *macro_call,
                           std::vector<ast::Expression *> arguments) override;
};

#endif //TARIK_SRC_SEMANTIC_MACRO_H_

// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Macro.h"

#include "syntactic/ast/Expression.h"

CastMacro::CastMacro() {
    arguments = {EXPRESSION, TYPE};
}

ast::Expression *CastMacro::apply(ast::Expression *macro_call, std::vector<ast::Expression *> arguments) {
    Type target = ((ast::TypeExpression *) arguments[1])->type;

    return new ast::CastExpression(macro_call->origin, arguments[0], target);
}

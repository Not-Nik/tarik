// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Macro.h"

#include <ranges>

#include "Analyser.h"
#include "syntactic/ast/Expression.h"

CastMacro::CastMacro() {
    arguments = {EXPRESSION, TYPE};
}

ast::Expression *CastMacro::apply(Analyser *analyser,
                                  ast::Expression *macro_call,
                                  std::vector<ast::Expression *> arguments) {
    Type target = ((ast::TypeExpression *) arguments[1])->type;

    return new ast::CastExpression(macro_call->origin, arguments[0], target);
}

template <bool VARIABLE_ARGS>
ExternMacro<VARIABLE_ARGS>::ExternMacro() {
    arguments = {TYPE, IDENTIFIER, TYPE, REPEAT};
}

template <bool VARIABLE_ARGS>
ast::Expression *ExternMacro<VARIABLE_ARGS>::apply(Analyser *analyser,
                                                   ast::Expression *macro_call,
                                                   std::vector<ast::Expression *> arguments) {
    Type return_type = analyser->verify_type(((ast::TypeExpression *) arguments[0])->type).value_or(Type());
    Path func_path = Path::from_expression(arguments[1]);
    std::vector<aast::VariableStatement *> func_args;
    for (ast::Expression *argument : std::ranges::drop_view {arguments, 2}) {
        Type arg_type = analyser->verify_type(((ast::TypeExpression *) argument)->type).value_or(Type());

        aast::VariableStatement *var = new aast::VariableStatement(argument->origin,
                                                                   arg_type,
                                                                   Token::name("_", argument->origin));

        func_args.push_back(var);
    }

    func_path = func_path.with_prefix(analyser->path);

    if (aast::FuncDeclareStatement *ex = analyser->get_func_decl(func_path)) {
        analyser->bucket->error(func_path.origin, "redefinition of '{}'", func_path.str())
                ->note(ex->path.origin, "previous definition here");
    }

    analyser->func_decls.emplace(func_path,
                                 new aast::FuncDeclareStatement(macro_call->origin,
                                                                func_path,
                                                                return_type,
                                                                func_args,
                                                                VARIABLE_ARGS));

    return new ast::EmptyExpression(macro_call->origin);
}

template class ExternMacro<true>;
template class ExternMacro<false>;

// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Analyser.h"

#include <algorithm>
#include <iostream>

#include "error/Error.h"
#include "Path.h"
#include "semantic/ast/Expression.h"
#include "syntactic/ast/Expression.h"
#include "Variables.h"

Analyser::Analyser(Bucket *bucket)
    : bucket(bucket),
      macros({{"as!", new CastMacro()}}) {}

std::vector<aast::Statement *> Analyser::finish() {
    std::vector<aast::Statement *> res;
    res.reserve(structures.size() + func_decls.size() + functions.size());
    for (auto [_, st] : structures)
        res.push_back(st);

    std::sort(res.begin(),
              res.end(),
              [](aast::Statement *s1, aast::Statement *s2) {
                  return !(s1->origin > s2->origin);
              });

    for (auto [_, dc] : func_decls)
        res.push_back(dc);
    for (auto *fn : functions)
        res.push_back(fn);
    return res;
}

void Analyser::analyse(const std::vector<ast::Statement *> &statements) {
    analyse_import(statements);
    verify_statements(statements);
}

void Analyser::analyse_import(const std::vector<ast::Statement *> &statements) {
    for (auto *statement : statements) {
        if (statement->statement_type == ast::FUNC_STMT) {
            auto *func = (ast::FuncStatement *) (statement);
            Path name = Path({});
            if (func->member_of.has_value()) {
                name = func->member_of.value().get_path().with_prefix(path).create_member(func->name.raw);
            } else {
                name = path.create_member(func->name);
            }

            std::vector<aast::VariableStatement *> arguments;

            for (auto *var : func->arguments) {
                arguments.push_back(new aast::VariableStatement(var->origin, var->type, var->name));
            }

            func_decls.emplace(name,
                                 new aast::FuncDeclareStatement(statement->origin,
                                                                name,
                                                                func->return_type,
                                                                arguments,
                                                                func->var_arg));
        } else if (statement->statement_type == ast::STRUCT_STMT) {
            auto *struct_ = (ast::StructStatement *) statement;

            Path name = path.create_member(struct_->name);

            struct_decls.emplace(name, new aast::StructDeclareStatement(statement->origin, name));
        } else if (statement->statement_type == ast::IMPORT_STMT) {
            auto *import_ = (ast::ImportStatement *) statement;

            path = path.create_member(import_->name);
            analyse_import(import_->block);
            path = path.get_parent();
        }
    }
}

std::optional<aast::Statement *> Analyser::verify_statement(ast::Statement *statement) {
    bool allowed = true;

    switch (statement->statement_type) {
        case ast::STRUCT_STMT:
        case ast::IMPORT_STMT:
        case ast::FUNC_STMT:
            allowed = bucket->error(statement->origin, "declaration not allowed here")
                            ->assert(level == 0);
            break;
        case ast::IF_STMT:
        case ast::ELSE_STMT:
        case ast::RETURN_STMT:
        case ast::WHILE_STMT:
        case ast::BREAK_STMT:
        case ast::CONTINUE_STMT:
        case ast::VARIABLE_STMT:
        case ast::EXPR_STMT:
        case ast::SCOPE_STMT:
            allowed = bucket->error(statement->origin, "expected declaration")
                            ->assert(level > 0);
            [[fallthrough]];
        default:
            break;
    }

    if (!allowed)
        return {};

    switch (statement->statement_type) {
        case ast::SCOPE_STMT:
            return verify_scope((ast::ScopeStatement *) statement);
        case ast::FUNC_STMT:
            return verify_function((ast::FuncStatement *) statement);
        case ast::IF_STMT:
            return verify_if((ast::IfStatement *) statement);
        case ast::ELSE_STMT:
            return verify_else((ast::ElseStatement *) statement);
        case ast::RETURN_STMT:
            return verify_return((ast::ReturnStatement *) statement);
        case ast::WHILE_STMT:
            return verify_while((ast::WhileStatement *) statement);
        case ast::BREAK_STMT:
            return verify_break((ast::BreakStatement *) statement);
        case ast::CONTINUE_STMT:
            return verify_continue((ast::ContinueStatement *) statement);
        case ast::VARIABLE_STMT: {
            std::optional sem = verify_variable((ast::VariableStatement *) statement);
            if (sem.has_value())
                return sem.value()->var;
            break;
        }
        case ast::STRUCT_STMT:
            return verify_struct((ast::StructStatement *) statement);
        case ast::IMPORT_STMT:
            return verify_import((ast::ImportStatement *) statement);
        case ast::EXPR_STMT:
            return verify_expression((ast::Expression *) statement);
    }
    return {};
}

std::optional<std::vector<aast::Statement *>> Analyser::verify_statements(
    const std::vector<ast::Statement *> &statements) {
    std::vector<aast::Statement *> res;

    for (auto *statement : statements) {
        std::optional verified = verify_statement(statement);
        if (verified.has_value())
            res.push_back(verified.value());
    }
    return res;
}

std::optional<aast::ScopeStatement *> Analyser::verify_scope(ast::ScopeStatement *scope, std::string name) {
    size_t old_var_count = variables.size();

    for (auto *var : variables) {
        var->push_state(*var->state());
        if (var->state()->is_definitely_defined())
            var->state()->make_definitely_read();
    }

    path = path.create_member(name);

    level++;

    std::optional statements = verify_statements(scope->block);

    level--;

    while (old_var_count < variables.size())
        variables.pop_back();

    // todo: for loops this doesn't catch all cases where defines at the start immediately follow defines at the end
    for (auto *var : variables) {
        VariableState old_definite_state = *var->state();
        var->pop_state();

        if (scope->statement_type == ast::SCOPE_STMT) {
            // plain scopes are always executed, so the old state is the new one
            var->pop_state();
            var->push_state(old_definite_state);
        } else {
            VariableState new_definite_state = *var->state() || old_definite_state;
            var->pop_state();
            var->push_state(new_definite_state);
        }
    }

    path = path.get_parent();

    aast::StmtType statement_type;

    // aaaaaaarghhhh
    switch (scope->statement_type) {
        case ast::SCOPE_STMT:
            statement_type = aast::SCOPE_STMT;
            break;
        case ast::FUNC_STMT:
            statement_type = aast::FUNC_STMT;
            break;
        case ast::IF_STMT:
            statement_type = aast::IF_STMT;
            break;
        case ast::ELSE_STMT:
            statement_type = aast::ELSE_STMT;
            break;
        case ast::WHILE_STMT:
            statement_type = aast::WHILE_STMT;
            break;
        case ast::IMPORT_STMT:
            statement_type = aast::IMPORT_STMT;
            break;
        // These aren't even scopes, so fail
        case ast::RETURN_STMT:
        case ast::BREAK_STMT:
        case ast::CONTINUE_STMT:
        case ast::VARIABLE_STMT:
        case ast::STRUCT_STMT:
        case ast::EXPR_STMT:
            return {};
    }

    if (statements.has_value())
        return new aast::ScopeStatement(statement_type, scope->origin, statements.value());
    else
        return {};
}

std::optional<aast::FuncStatement *> Analyser::verify_function(ast::FuncStatement *func) {
    variables.clear();
    variable_names.clear();
    used_names.clear();
    last_loop = nullptr; // this shouldn't do anything, but just to be sure

    if (func->member_of.has_value())
        verify_type(func->member_of.value());

    Path func_path = Path({});
    if (func->member_of.has_value()) {
        func_path = func->member_of.value().get_path().with_prefix(path).create_member(func->name.raw);
    } else {
        func_path = path.create_member(func->name);
    }

    for (auto *registered : functions) {
        bucket->error(func->name.origin, "redefinition of '{}'", func->name.raw)
              ->note(registered->path.origin, "previous definition here")
              ->assert(func_path != registered->path);
    }

    bucket->error(func->return_type.origin, "function with return type doesn't always return")
          ->assert(func->return_type == Type(VOID) || does_always_return(func));
    std::vector<aast::VariableStatement *> arguments;

    for (auto *arg : func->arguments) {
        std::optional sem = verify_variable(arg);

        if (sem.has_value())
            sem.value()->state()->make_definitely_defined(arg->origin);

        arguments.push_back(sem.value()->var);
    }

    if (func->var_arg)
        bucket->warning(func->origin, "function uses var args, but they cannot be accessed");

    std::optional return_type = verify_type(func->return_type);

    if (return_type.has_value()) {
        this->return_type = return_type.value();
        std::optional scope = verify_scope(func, func->name.raw);
        if (scope.has_value()) {
            std::vector block = std::move(scope.value()->block);
            delete scope.value();
            functions.push_back(new aast::FuncStatement(func->origin,
                                                        func_path,
                                                        return_type.value(),
                                                        arguments,
                                                        block,
                                                        func->var_arg));

            return functions.back();
        }
    }
    return {};
}

std::optional<aast::IfStatement *> Analyser::verify_if(ast::IfStatement *if_) {
    std::optional condition = verify_expression(if_->condition);
    std::optional scope = verify_scope(if_);
    std::optional<aast::ScopeStatement *> else_ = {};
    if (if_->else_statement)
        else_ = verify_scope(if_->else_statement);

    if (condition.has_value() && scope.has_value()) {
        // implicitely compare to 0 if condition isn't a bool already
        if (condition.value()->type != Type(BOOL, 0)) {
            condition = new aast::BinaryExpression(if_->condition->origin,
                                                   Type(BOOL),
                                                   aast::NEQ,
                                                   condition.value(),
                                                   new aast::IntExpression(if_->condition->origin, 0));
        }

        std::vector block = std::move(scope.value()->block);
        delete scope.value();
        auto new_if = new aast::IfStatement(if_->origin, condition.value(), block);

        if (if_->else_statement && else_.has_value()) {
            new_if->else_statement = new aast::ElseStatement(if_->else_statement->origin, else_.value()->block);
            else_.value()->block.clear();
            delete else_.value();
        } else {
            return {};
        }
        return new_if;
    } else {
        return {};
    }
}

std::optional<aast::ElseStatement *> Analyser::verify_else(ast::ElseStatement *else_) {
    bucket->error(else_->origin, "else, but no preceding if");
    return {};
}

std::optional<aast::ReturnStatement *> Analyser::verify_return(ast::ReturnStatement *return_) {
    if (return_->value) {
        std::optional value = verify_expression(return_->value);
        if (value.has_value()) {
            bucket->error(return_->value->origin,
                          "can't return value of type '{}' in function with return type '{}'",
                          value.value()->type.str(),
                          return_type.str())
                  ->assert(return_type.is_assignable_from(value.value()->type));

            return new aast::ReturnStatement(return_->origin, value.value());
        }
    } else {
        bucket->error(return_->origin, "function with return type should return a value")
              ->assert(return_type == Type(VOID));

        return new aast::ReturnStatement(return_->origin, nullptr);
    }

    return {};
}

std::optional<aast::WhileStatement *> Analyser::verify_while(ast::WhileStatement *while_) {
    ast::Statement *old_last_loop = last_loop;
    last_loop = while_;

    std::optional condition = verify_expression(while_->condition);
    std::optional scope = verify_scope(while_);
    last_loop = old_last_loop;

    if (condition.has_value() && scope.has_value()) {
        // implicitely compare to 0 if condition isn't a bool already
        if (condition.value()->type != Type(BOOL, 0)) {
            condition = new aast::BinaryExpression(while_->condition->origin,
                                                   Type(BOOL),
                                                   aast::NEQ,
                                                   condition.value(),
                                                   new aast::IntExpression(while_->condition->origin, 0));
        }

        std::vector block = std::move(scope.value()->block);
        delete scope.value();
        return new aast::WhileStatement(while_->origin, condition.value(), block);
    } else {
        return {};
    }
}

std::optional<aast::BreakStatement *> Analyser::verify_break(ast::BreakStatement *break_) {
    bucket->error(break_->origin, "break outside of loop")
          ->assert(last_loop);
    return new aast::BreakStatement(break_->origin);
}

std::optional<aast::ContinueStatement *> Analyser::verify_continue(ast::ContinueStatement *continue_) {
    bucket->error(continue_->origin, "continue outside of loop")
          ->assert(last_loop);
    return new aast::ContinueStatement(continue_->origin);
}

std::optional<SemanticVariable *> Analyser::verify_variable(ast::VariableStatement *var) {
    std::optional type = verify_type(var->type);

    for (auto *variable : variables) {
        bucket->error(var->name.origin, "redefinition of '{}'", var->name.raw)
              ->note(variable->var->name.origin, "previous definition here")
              ->assert(var->name.raw != variable->var->name.raw);
    }

    if (!type.has_value())
        return {};

    Token name = var->name;
    // If the name was used before, i.e. in a previous, separate scope
    if (used_names.contains(name.raw)) {
        // Find a replacement that wasn't
        for (std::size_t i = 1; i < std::numeric_limits<std::size_t>::max(); i++) {
            if (!used_names.contains(name.raw + std::to_string(i))) {
                name.raw += std::to_string(i);
                break;
            }
        }
    }
    // Store that we used the name before
    used_names.emplace(name.raw);
    variable_names[var->name.raw] = name.raw;

    auto *new_var = new aast::VariableStatement(var->origin, type.value(), name);

    SemanticVariable *sem;
    if (type.value().is_primitive()) {
        sem = new PrimitiveVariable(new_var);
    } else {
        aast::StructStatement *st = get_struct(type.value().get_user());
        if (!st)
            return {};

        std::vector<SemanticVariable *> member_states;
        for (auto *member : st->members) {
            auto *temp = new ast::VariableStatement(var->origin,
                                                    member->type,
                                                    Token::name(name.raw + "." + member->name.raw));

            std::optional semantic_member = verify_variable(temp);
            if (semantic_member.has_value())
                member_states.push_back(semantic_member.value());
        }

        sem = new CompoundVariable(new_var, member_states);
    }
    variables.push_back(sem);
    return {sem};
}

std::optional<aast::StructStatement *> Analyser::verify_struct(ast::StructStatement *struct_) {
    std::vector<std::string> registered;
    Path struct_path = Path({struct_->name.raw}, struct_->name.origin).with_prefix(path);

    for (const auto &[path, registered] : structures) {
        bucket->error(struct_->name.origin, "redefinition of '{}'", struct_->name.raw)
              ->note(registered->path.origin, "previous definition here")
              ->assert(path != struct_path);
    }

    std::vector<aast::VariableStatement *> members;
    std::vector<ast::VariableStatement *> ctor_args;
    std::vector<ast::Statement *> body;

    auto *instance = new ast::VariableStatement(struct_->origin, struct_->get_type(path), Token::name("_instance"));
    body.push_back(instance);

    for (auto *member : struct_->members) {
        bucket->error(member->name.origin, "duplicate member '{}'", member->name.raw)
              ->assert(std::find(registered.begin(), registered.end(), member->name.raw) == registered.end());

        std::optional member_type = verify_type(member->type);

        registered.push_back(member->name.raw);
        if (member_type.has_value()) {
            members.push_back(new aast::VariableStatement(member->origin, member_type.value(), member->name));
            ctor_args.push_back(new ast::VariableStatement(struct_->origin, member_type.value(), member->name));

            auto *member_access = new ast::BinaryExpression(struct_->origin,
                                                            ast::MEM_ACC,
                                                            new ast::NameExpression(
                                                                struct_->origin,
                                                                "_instance"),
                                                            new ast::NameExpression(
                                                                struct_->origin,
                                                                member->name.raw));

            body.push_back(new ast::BinaryExpression(struct_->origin,
                                                     ast::ASSIGN,
                                                     member_access,
                                                     new ast::NameExpression(
                                                         struct_->origin,
                                                         member->name.raw)));
        }
    }
    body.push_back(new ast::ReturnStatement(struct_->origin, new ast::NameExpression(struct_->origin, "_instance")));

    Path constructor_path = struct_path.create_member("$constructor");
    auto *ctor = new ast::FuncStatement(struct_->origin,
                                        Token::name("$constructor", struct_->origin),
                                        struct_->get_type(path),
                                        ctor_args,
                                        body,
                                        false,
                                        {});

    auto *new_struct = new aast::StructStatement(struct_->origin, struct_path, members);
    structures.emplace(struct_path, new_struct);

    path = path.create_member(struct_->name);
    // fixme: this throws additional errors if member types are invalid
    std::optional constructor = verify_function(ctor);
    path = path.get_parent();

    bucket->error(struct_->origin, "internal: failed to verify constructor for '{}'", struct_path.str())
          ->assert(constructor.has_value());
    func_decls.emplace(constructor_path,
                         new aast::FuncDeclareStatement(ctor->origin,
                                                        constructor_path,
                                                        ctor->return_type,
                                                        constructor.value()->arguments,
                                                        ctor->var_arg));

    return new_struct;
}

std::optional<aast::ImportStatement *> Analyser::verify_import(ast::ImportStatement *import_) {
    path = path.create_member(import_->name);
    std::optional res = verify_statements(import_->block);
    path = path.get_parent();

    if (res.has_value())
        return new aast::ImportStatement(import_->origin, import_->name, res.value());
    else
        return {};
}

std::optional<aast::Expression *> Analyser::verify_expression(ast::Expression *expression,
                                                              AccessType access,
                                                              bool member_acc) {
    switch (expression->expression_type) {
        case ast::CALL_EXPR:
            return verify_call_expression(expression, access, member_acc);
        case ast::DASH_EXPR:
        case ast::DOT_EXPR:
        case ast::EQ_EXPR:
        case ast::COMP_EXPR:
        case ast::ASSIGN_EXPR:
            return verify_binary_expression(expression, access, member_acc);
        case ast::MEM_ACC_EXPR:
            return verify_member_access_expression(expression, access, member_acc);
        case ast::PREFIX_EXPR:
            return verify_prefix_expression(expression, access, member_acc);
        case ast::NAME_EXPR:
            return verify_name_expression(expression, access, member_acc);
        case ast::INT_EXPR:
            return new aast::IntExpression(expression->origin, ((ast::IntExpression *) expression)->n);
        case ast::REAL_EXPR:
            return new aast::RealExpression(expression->origin, ((ast::RealExpression *) expression)->n);
        case ast::STR_EXPR:
            return new aast::StringExpression(expression->origin, ((ast::StringExpression *) expression)->n);
        case ast::BOOL_EXPR:
            return new aast::BoolExpression(expression->origin, ((ast::BoolExpression *) expression)->n);
        case ast::PATH_EXPR: {
            Error *error = bucket->error(expression->origin, "unexpected path expression");
            Path path = Path::from_expression(expression);

            if (is_func_declared(path))
                error->note(expression->origin, "did you mean to call '{}'?", path.str());

            if (is_struct_declared(path))
                error->note(expression->origin, "did you mean to create a variable with type '{}'?", path.str());

            break;
        }
        case ast::CAST_EXPR: {
            auto *ce = (ast::CastExpression *) expression;
            std::optional expr = verify_expression(ce->expression);

            if (expr.has_value()) {
                Error *error = bucket->error(expr.value()->origin, "can't cast between compound types");
                if (expr.value()->type.pointer_level == 0)
                    error->note(expr.value()->origin,
                                "you should create an as_{} function in {} to perform this conversion",
                                ce->target_type.func_name(),
                                expr.value()->type.str());
                error->assert(expr.value()->type.is_primitive() && ce->target_type.is_primitive());

                return new aast::CastExpression(expression->origin, expr.value(), ce->target_type);
            }
            return {};
        }
        default:
            // todo: do more analysis here:
            //  if it's a path, put an error after the expression: if the path points to a valid function, suggest
            //  calling it. if it points to a valid struct, suggest creating a variable with it
            bucket->error(expression->origin, "internal: unexpected, unhandled type of expression");
    }

    return
            {};
}

std::optional<aast::Expression *> Analyser::verify_call_expression(ast::Expression *expression,
                                                                   AccessType access,
                                                                   bool member_acc) {
    auto *ce = (ast::CallExpression *) expression;

    Path func_path = Path({});
    std::vector<aast::Expression *> arguments;

    if (auto *gl = (ast::PrefixExpression *) ce->callee;
        ce->callee->expression_type == ast::NAME_EXPR || ce->callee->expression_type == ast::PATH_EXPR ||
        (ce->callee->expression_type == ast::PREFIX_EXPR && gl->prefix_type == ast::GLOBAL)) {
        func_path = Path::from_expression(ce->callee);

        if (is_struct_declared(func_path)) {
            func_path = func_path.create_member("$constructor");
            // TODO: check if there exists a function with the same name
        }
    } else if (ce->callee->expression_type == ast::MEM_ACC_EXPR) {
        auto *mem = (ast::BinaryExpression *) ce->callee;
        std::optional callee_parent = verify_expression(mem->left);

        if (mem->right->expression_type == ast::MACRO_NAME_EXPR) {
            return verify_macro_expression(expression, access, member_acc);
        } else if (mem->right->expression_type != ast::NAME_EXPR) {
            bucket->error(mem->right->origin, "expected function name");
            return {};
        }

        if (!callee_parent.has_value())
            return {};

        std::string member_name = ((ast::NameExpression *) mem->right)->name;

        if (callee_parent.value()->type.is_primitive() && callee_parent.value()->type.get_primitive() == U0) {
            std::vector possible_types = {U8, U16, U32, U64};
            auto *ie = (ast::IntExpression *) callee_parent.value();
            if (ie->n < 0)
                possible_types.insert(possible_types.end(), {I8, I16, I32, I64});

            std::vector<TypeSize> types_with_matching_functions;
            for (auto type : possible_types) {
                Type dummy = Type(type, callee_parent.value()->type.pointer_level);
                if (is_func_declared(dummy.get_path().create_member(member_name))) {
                    types_with_matching_functions.push_back(type);
                }
            }

            Error *error = bucket->error(ce->origin, "ambigious function call");
            for (auto type : types_with_matching_functions) {
                Type dummy = Type(type, callee_parent.value()->type.pointer_level);
                error->note(get_func_decl(dummy.get_path().create_member(member_name))->origin,
                            "possible candidate function here");
            }
            if (!error->assert(types_with_matching_functions.size() <= 1))
                return {};

            if (types_with_matching_functions.size() == 1) {
                callee_parent.value()->type = Type(types_with_matching_functions[0],
                                                   callee_parent.value()->type.pointer_level);
            }
        }

        func_path = callee_parent.value()->type.get_path();
        func_path = func_path.create_member(member_name);

        if (callee_parent.value()->type.pointer_level > 0) {
            Type dummy = callee_parent.value()->type.get_deref();
            Path deref_func = dummy.get_path().create_member(member_name);
            if (is_func_declared(deref_func)) {
                aast::FuncDeclareStatement *decl = get_func_decl(deref_func);
                if (decl->arguments[0]->name.raw == "this" &&
                    decl->arguments[0]->type == callee_parent.value()->type) {
                    if (is_func_declared(func_path)) {
                        bucket->error(ce->origin, "ambigious function call")
                              ->note(get_func_decl(func_path)->origin, "possible candidate function here")
                              ->note(decl->origin, "possible candidate function here");
                        return {};
                    }
                    func_path = deref_func;
                }
            }
        }

        arguments.push_back(callee_parent.value());
    } else if (ce->callee->expression_type == ast::MACRO_NAME_EXPR) {
        return verify_macro_expression(expression, access, member_acc);
    } else {
        bucket->error(ce->callee->origin, "calling of expressions is unimplemented");
        return {};
    }

    if (!bucket->error(ce->callee->origin, "undefined function '{}'", func_path.str())
               ->assert(is_func_declared(func_path)))
        return {};
    aast::FuncStCommon *func = get_func_decl(func_path);
    Path func_parent = func->path.get_parent();

    size_t arg_offset = 0;
    if (is_struct_declared(func_parent) ||
        to_typesize(func_parent.str()) != (TypeSize) -1 ||
        func->path.contains_pointer()) {
        if (func->arguments.size() > 0 && func->arguments[0]->name.raw == "this") {
            if (func->arguments[0]->type.pointer_level == arguments[0]->type.pointer_level + 1)
                arguments[0] = new aast::PrefixExpression(arguments[0]->origin,
                                                          arguments[0]->type.get_pointer_to(),
                                                          aast::REF,
                                                          arguments[0]);
            else if (arguments[0]->flattens_to_member_access()) {
                get_variable(arguments[0]->flatten_to_member_access())->state()->make_definitely_moved(
                    arguments[0]->origin);
            }

            bucket->error(arguments[0]->origin,
                          "passing value of type '{}' to argument of type '{}'",
                          arguments[0]->type.str(),
                          func->arguments[0]->type.str())
                  ->assert(func->arguments[0]->type.is_assignable_from(arguments[0]->type));
            arg_offset = 1;
        } else if (func->path.get_parts().back() != "$constructor")
            // constructors are in the scope of a struct, but aren't called by a member expression. Otherwise
            // function is static, but we always put in the this argument
            arguments.erase(arguments.begin());
    }

    if (!bucket->error(ce->origin,
                       "too few arguments, expected {} found {}",
                       func->arguments.size() - arg_offset,
                       ce->arguments.size())
               ->assert(ce->arguments.size() >= func->arguments.size() - arg_offset))
        return {};

    if (!bucket->error(ce->origin,
                       "too many arguments, expected {} found {}",
                       func->arguments.size() - arg_offset,
                       ce->arguments.size())
               ->assert(func->var_arg || ce->arguments.size() <= func->arguments.size() - arg_offset))
        return {};

    for (int i = arg_offset; i < std::min(func->arguments.size(), ce->arguments.size() + arg_offset); i++) {
        aast::VariableStatement *arg_var = func->arguments[i];
        ast::Expression *arg = ce->arguments[i - arg_offset];

        std::optional argument = verify_expression(arg, MOVE);

        if (argument.has_value()) {
            if (arg_var->type.is_float() && argument.value()->expression_type == aast::INT_EXPR) {
                auto *real = new aast::RealExpression(argument.value()->origin,
                                                      (double) ((aast::IntExpression *) argument.value())->n);
                delete argument.value();
                argument = real;
            }

            arguments.push_back(argument.value());

            bucket->error(arg->origin,
                          "passing value of type '{}' to argument of type '{}'",
                          argument.value()->type.str(),
                          arg_var->type.str())
                  ->assert(arg_var->type.is_assignable_from(argument.value()->type));
        }
    }

    // fixme: this needs to be a path expression kind of thing
    // fixme: in the very far future, this should have a function pointer type
    auto *callee = new aast::NameExpression(ce->callee->origin, Type(), func_path.str());

    return new aast::CallExpression(expression->origin, func->return_type, callee, arguments);
}

std::optional<aast::Expression *> Analyser::verify_macro_expression(ast::Expression *expression,
                                                                    AccessType access,
                                                                    bool member_acc) {
    auto *ce = (ast::CallExpression *) expression;
    if (ce->callee->expression_type == ast::MEM_ACC_EXPR) {
        auto *mem = (ast::BinaryExpression *) ce->callee;
        ce->arguments.insert(ce->arguments.begin(), mem->left);

        ast::Expression *callee = mem->right;
        mem->right = nullptr;
        mem->left = nullptr;
        delete mem;

        ce->callee = callee;
    }

    Macro *macro = macros[((ast::MacroNameExpression *) ce->callee)->name];

    if (!bucket->error(ce->origin,
                       "too few arguments, expected {} found {}",
                       macro->arguments.size(),
                       ce->arguments.size())
               ->assert(ce->arguments.size() >= macro->arguments.size()))
        return {};

    if (!bucket->error(ce->origin,
                       "too many arguments, expected {} found {}",
                       macro->arguments.size(),
                       ce->arguments.size())
               ->assert(ce->arguments.size() <= macro->arguments.size()))
        return {};

    for (int i = 0; i < macro->arguments.size(); i++) {
        ast::Expression *arg = ce->arguments[i];
        if (macro->arguments[i] == Macro::IDENTIFIER) {
            if (!bucket->error(arg->origin, "expected identifier")
                       ->assert(arg->expression_type == ast::NAME_EXPR ||
                                arg->expression_type == ast::PATH_EXPR ||
                                (arg->expression_type == ast::PREFIX_EXPR &&
                                 ((ast::PrefixExpression *) arg)->prefix_type == ast::GLOBAL)))
                return {};
        } else if (macro->arguments[i] == Macro::TYPE) {
            if (!bucket->error(arg->origin, "expected type")
                       ->assert(arg->expression_type == ast::TYPE_EXPR))
                return {};
        }
    }

    return verify_expression(macro->apply(ce, ce->arguments));
}


std::optional<aast::BinaryExpression *> Analyser::verify_binary_expression(
    ast::Expression *expression,
    AccessType access,
    bool member_acc) {
    auto *ae = (ast::BinaryExpression *) expression;

    access = expression->expression_type == ast::ASSIGN_EXPR ? ASSIGNMENT : NORMAL;
    std::optional<aast::Expression *> left = {};
    std::optional<aast::Expression *> right = {};

    if (expression->expression_type == ast::ASSIGN_EXPR) {
        right = verify_expression(ae->right);
        left = verify_expression(ae->left, access);
    } else {
        left = verify_expression(ae->left, access);
        right = verify_expression(ae->right);
    }

    if (!left.has_value() || !right.has_value())
        return {};

    if (left.value()->type.is_float() && right.value()->expression_type == aast::INT_EXPR) {
        auto *real = new aast::RealExpression(right.value()->origin,
                                              (double) ((aast::IntExpression *) right.value())->n);
        delete right.value();
        right = real;
    }

    if (right.value()->type.is_float() && left.value()->expression_type == aast::INT_EXPR) {
        auto *real = new aast::RealExpression(left.value()->origin,
                                              (double) ((aast::IntExpression *) left.value())->n);
        delete left.value();
        left = real;
    }

    if (ae->expression_type == ast::ASSIGN_EXPR) {
        bucket->error(ae->right->origin,
                      "can't assign to type '{}' from '{}'",
                      left.value()->type.str(),
                      right.value()->type.str())
              ->assert(left.value()->type.is_assignable_from(right.value()->type));
    } else if (ae->expression_type == ast::EQ_EXPR || ae->expression_type == ast::COMP_EXPR) {
        bucket->error(ae->origin,
                      "invalid operands to binary expression '{}' and '{}'",
                      left.value()->type.str(),
                      right.value()->type.str())
              ->assert(left.value()->type.is_comparable(right.value()->type));
    } else /*if (math expression)*/ {
        bucket->error(ae->origin,
                      "invalid operands to binary expression '{}' and '{}'",
                      left.value()->type.str(),
                      right.value()->type.str())
              ->assert(left.value()->type.is_compatible(right.value()->type));
    }

    Type expression_type;
    if (expression->expression_type == ast::EQ_EXPR || expression->expression_type == ast::COMP_EXPR)
        expression_type = Type(BOOL);
    else if (expression->expression_type == ast::ASSIGN_EXPR)
        expression_type = left.value()->type;
    else
        expression_type = left.value()->type.get_result(right.value()->type);

    aast::BinOpType bot;

    switch (ae->bin_op_type) {
        case ast::PATH:
            break;
        case ast::ADD:
            bot = aast::ADD;
            break;
        case ast::SUB:
            bot = aast::SUB;
            break;
        case ast::MUL:
            bot = aast::MUL;
            break;
        case ast::DIV:
            bot = aast::DIV;
            break;
        case ast::EQ:
            bot = aast::EQ;
            break;
        case ast::NEQ:
            bot = aast::NEQ;
            break;
        case ast::SM:
            bot = aast::SM;
            break;
        case ast::GR:
            bot = aast::GR;
            break;
        case ast::SME:
            bot = aast::SME;
            break;
        case ast::GRE:
            bot = aast::GRE;
            break;
        case ast::MEM_ACC:
            break;
        case ast::ASSIGN:
            bot = aast::ASSIGN;
            break;
    }

    return new aast::BinaryExpression(ae->origin,
                                      expression_type,
                                      bot,
                                      left.value(),
                                      right.value());
}

std::optional<aast::BinaryExpression *> Analyser::verify_member_access_expression(
    ast::Expression *expression,
    AccessType access,
    bool member_acc) {
    auto *mae = (ast::BinaryExpression *) expression;

    std::optional left = verify_expression(mae->left, access, true);

    if (!left.has_value())
        return {};

    if (!bucket->error(left.value()->origin, "'{}' is not a structure", left.value()->type.str())
               ->assert(!left.value()->type.is_primitive()))
        return {};
    Path struct_name = left.value()->type.get_user();
    aast::StructStatement *st = get_struct(struct_name);
    if (!bucket->error(left.value()->origin, "undefined structure '{}'", left.value()->type.str())
               ->assert(st))
        return {};

    if (!bucket->error(mae->right->origin, "expected identifier")
               ->assert(mae->right->expression_type == ast::NAME_EXPR))
        return {};

    std::string member_name = ((ast::NameExpression *) mae->right)->name;

    if (!bucket->error(mae->right->origin,
                       "no member named '{}' in '{}'",
                       member_name,
                       left.value()->type.str())
               ->assert(st->has_member(member_name)))
        return {};

    Type member_type = st->get_member_type(member_name);

    if (left.value()->flattens_to_member_access()) {
        auto *name = new ast::NameExpression(mae->origin,
                                             left.value()->flatten_to_member_access() + "." + member_name);

        verify_name_expression(name, access);
        delete name;
    }

    return new aast::BinaryExpression(mae->origin,
                                      member_type,
                                      aast::MEM_ACC,
                                      left.value(),
                                      new
                                      aast::NameExpression(mae->right->origin, Type(), member_name));
}

std::optional<aast::PrefixExpression *> Analyser::verify_prefix_expression(
    ast::Expression *expression,
    AccessType access,
    bool member_acc) {
    auto *pe = (ast::PrefixExpression *) expression;
    std::optional operand = verify_expression(pe->operand);

    if (!operand.has_value())
        return {};

    Type pe_type = operand.value()->type;

    switch (pe->prefix_type) {
        case ast::NEG:
            bucket->error(pe->operand->origin, "invalid operand to prefix expression")
                  ->assert(pe_type.pointer_level == 0);
        case ast::LOG_NOT:
            bucket->error(pe->operand->origin, "invalid operand to prefix expression")
                  ->assert(pe_type.is_primitive() || pe_type.pointer_level > 0);
            break;
        case ast::REF: {
            pe_type.pointer_level++;
            break;
        }
        case ast::DEREF:
            bucket->error(pe->operand->origin, "cannot dereference non-pointer type '{}'", operand.value()->type.str())
                  ->assert(pe_type.pointer_level > 0);
            bucket->error(pe->operand->origin, "cannot copy non-primitive type '{}'", operand.value()->type.str())
                  ->assert(pe_type.is_primitive() || pe_type.pointer_level > 1);
            pe_type.pointer_level--;
            break;
        case ast::GLOBAL:
            bucket->error(pe->origin, "unexpected global path prefix")
                  ->note(pe->origin, "did you mean to call a function? ('{}()')", pe->print());
            break;
    }

    aast::PrefixType pt;

    switch (pe->prefix_type) {
        case ast::NEG:
            pt = aast::NEG;
            break;
        case ast::REF:
            pt = aast::REF;
            break;
        case ast::DEREF:
            pt = aast::DEREF;
            break;
        case ast::LOG_NOT:
            pt = aast::LOG_NOT;
            break;
        case ast::GLOBAL:
            break;
    }

    return new aast::PrefixExpression(pe->origin, pe_type, pt, operand.value());
}

std::optional<aast::NameExpression *> Analyser::verify_name_expression(ast::Expression *expression,
                                                                       AccessType access,
                                                                       bool member_acc) {
    auto *ne = (ast::NameExpression *) expression;
    if (!bucket->error(expression->origin, "undefined variable '{}'", ne->name)
               ->assert(is_var_declared(ne->name)))
        return {};

    SemanticVariable *var = get_variable(ne->name);

    Type variable_type = var->var->type;

    // Use the name of the variable, which might not be the one in the NameExpression, if the name was replaced to avoid
    // having the same variable name in a function twice (even if they are in separate scopes)
    auto *new_expr = new aast::NameExpression(ne->origin, variable_type, var->var->name.raw);

    if (access == ASSIGNMENT)
        var->var->written_to = true;

    // don't do variable state checking if we are in a (pure) member access
    if (member_acc)
        return new_expr;

    if (access == ASSIGNMENT) {
        if (var->state()->is_definitely_defined()) {
            bucket->warning(var->state()->get_defined_pos(), "assigned value is immediately discarded")
                  ->note(expression->origin, "overwritten here");
        } else if (var->state()->is_maybe_defined()) {
            bucket->warning(expression->origin, "value might overwrite previous assignment without reading it");
        }

        var->state()->make_definitely_defined(expression->origin);
    } else if (bucket->error(expression->origin, "value is still undefined")
                     ->assert(!var->state()->is_definitely_undefined()) &&
               bucket->error(expression->origin, "value might be undefined")
                     ->assert(!var->state()->is_maybe_undefined()) &&
               bucket->error(expression->origin, "value was moved")
                     ->note(var->state()->get_moved_pos(), "moved here")
                     ->assert(!var->state()->is_definitely_moved()) &&
               bucket->error(expression->origin, "value might have been moved")
                     ->note(var->state()->get_moved_pos(), "moved here")
                     ->assert(!var->state()->is_maybe_moved())) {
        if (access == NORMAL) {
            var->state()->make_definitely_read();
        } else if (access == MOVE && !var->var->type.is_copyable()) {
            var->state()->make_definitely_moved(expression->origin);
        }
    }

    return new_expr;
}

std::optional<Type> Analyser::verify_type(Type type) {
    if (!type.is_primitive()) {
        Path path = type.get_user();
        if (!path.is_global()) {
            // global_path here, means global path to local struct
            // precisely, type->get_user() contains a local path
            // and global_path contains a global_path, both to the
            // same type. if type->get_user() interpreted locally
            // points to a type it is converted to a global path
            // and used, otherwise it is left as is and interpreted
            // as a global path
            Path global_path = path.with_prefix(this->path);
            if (is_struct_declared(global_path)) {
                path = global_path;
            }
        }

        bucket->error(type.origin, "undefied type '{}'", path.str())
              ->assert(is_struct_declared(path));

        type.set_user(path);
    }
    return type;
}

bool Analyser::does_always_return(ast::ScopeStatement *scope) {
    for (auto &it : scope->block) {
        if (it->statement_type == ast::RETURN_STMT || (
                it->statement_type == ast::SCOPE_STMT && does_always_return((ast::ScopeStatement *) it)) || (
                it->statement_type == ast::IF_STMT && ((ast::IfStatement *) it)->else_statement &&
                does_always_return((ast::ScopeStatement *) it) && does_always_return(
                    ((ast::IfStatement *) it)->else_statement))
            || (it->statement_type == ast::WHILE_STMT && does_always_return((ast::ScopeStatement *) it)))
            return true;
    }
    return false;
}

bool Analyser::is_var_declared(std::string name) const {
    if (variable_names.contains(name))
        name = variable_names.at(name);
    return std::find_if(variables.begin(),
                        variables.end(),
                        [name](SemanticVariable *v) {
                            return name == v->var->name.raw;
                        }) != variables.end();
}

bool Analyser::is_func_declared(Path path) const {
    if (path.is_global()) {
        return func_decls.contains(path);
    }

    return func_decls.contains(path) || func_decls.contains(path.with_prefix(this->path));
}

bool Analyser::is_struct_declared(Path path) const {
    if (path.is_global()) {
        return struct_decls.contains(path);
    }

    return struct_decls.contains(path) || struct_decls.contains(path.with_prefix(this->path));
}

SemanticVariable *Analyser::get_variable(std::string name) const {
    if (variable_names.contains(name))
        name = variable_names.at(name);
    return *std::find_if(variables.begin(),
                         variables.end(),
                         [name](SemanticVariable *v) {
                             return name == v->var->name.raw;
                         });
}

aast::FuncDeclareStatement *Analyser::get_func_decl(Path path) const {
    if (path.is_global()) {
        return func_decls.contains(path) ? func_decls.at(path) : nullptr;
    }

    Path local = path.with_prefix(this->path);

    if (func_decls.contains(local))
        return func_decls.contains(local) ? func_decls.at(local) : nullptr;

    return func_decls.contains(path) ? func_decls.at(path) : nullptr;
}

aast::StructStatement *Analyser::get_struct(Path path) const {
    if (path.is_global()) {
        return structures.contains(path) ? structures.at(path) : nullptr;
    }

    Path local = path.with_prefix(this->path);

    if (structures.contains(local))
        return structures.contains(local) ? structures.at(local) : nullptr;

    return structures.contains(path) ? structures.at(path) : nullptr;
}

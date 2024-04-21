// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Analyser.h"

#include <algorithm>
#include <iostream>

#include "Path.h"
#include "Variables.h"
#include "error/Error.h"
#include "syntactic/expressions/Expression.h"

#define UPDATE_RES(expr) {bool temp = expr; res = res && temp;}

std::vector<std::string> Analyser::get_global_path(const std::string &name) const {
    auto local = path;
    local.push_back(name);
    return local;
}

std::vector<std::string> Analyser::get_global_path(const std::vector<std::string> &name) const {
    auto local = path;
    local.insert(local.end(), name.begin(), name.end());
    return local;
}

std::string Analyser::flatten_path(const std::string &name) const {
    return flatten_path(path, name);
}

std::string Analyser::flatten_path(const std::vector<std::string> &path) {
    std::string res = "::";
    for (auto it = path.begin(); it != path.end();) {
        if (it->empty()) {
            it++;
            continue;
        }
        res += *it;
        if (++it != path.end()) {
            res += "::";
        }
    }
    return res;
}

std::string Analyser::flatten_path(std::vector<std::string> path, const std::string &name) {
    path.push_back(name);
    return flatten_path(path);
}

Analyser::Analyser(Bucket *bucket)
    : bucket(bucket) {
    LexerRange lr;
    std::vector<ast::Statement *> body;
    body.push_back(new ast::ReturnStatement(lr, new ast::CallExpression(lr, new ast::NameExpression(lr, "::main"), {})));

    auto *func = new ast::FuncStatement(lr, Token::name("main"), Type(U32), {}, body, false, {});
    auto *decl = new ast::FuncDeclareStatement(lr, Token::name("main"), Type(I32), {}, false, {});

    functions.emplace(std::vector<std::string>(), func);
    declarations.emplace(std::vector<std::string>(), decl);
}


std::vector<ast::Statement *> Analyser::finish() {
    std::vector<ast::Statement *> res;
    res.reserve(structures.size() + declarations.size() + functions.size());
    for (auto [_, st] : structures)
        res.push_back(st);
    for (auto [_, dc] : declarations)
        res.push_back(dc);
    for (auto [_, fn] : functions)
        res.push_back(fn);
    return res;
}

bool Analyser::analyse(const std::vector<ast::Statement *> &statements) {
    bool res;

    UPDATE_RES(analyse_import(statements))
    UPDATE_RES(verify_statements(statements))

    return res;
}

bool Analyser::analyse_import(const std::vector<ast::Statement *> &statements) {
    bool res = true;

    for (auto statement : statements) {
        UPDATE_RES(bucket->iassert(statement->statement_type != ast::FUNC_DECL_STMT,
            statement->origin,
            "internal: premature function declaration: report this as a bug"))

        if (statement->statement_type == ast::FUNC_STMT) {
            auto func = reinterpret_cast<ast::FuncStatement *>(statement);
            std::vector<std::string> name;
            if (func->member_of.has_value()) {
                name = get_global_path(func->member_of.value().get_path());
                name.push_back(func->name.raw);
            } else {
                name = get_global_path(func->name.raw);
            }
            declarations.emplace(name,
                                 new ast::FuncDeclareStatement(statement->origin,
                                                          Token::name(flatten_path(name), func->name.origin),
                                                          func->return_type,
                                                          func->arguments,
                                                          func->var_arg,
                                                          func->member_of));
        } else if (statement->statement_type == ast::IMPORT_STMT) {
            auto *import_ = (ast::ImportStatement *) statement;

            path.push_back(import_->name);
            UPDATE_RES(analyse_import(import_->block))
            path.pop_back();
        }
    }

    return res;
}

bool Analyser::verify_statement(ast::Statement *statement) {
    bool allowed = true;

    switch (statement->statement_type) {
        case ast::STRUCT_STMT:
        case ast::IMPORT_STMT:
        case ast::FUNC_STMT:
            allowed = bucket->iassert(level == 0, statement->origin, "declaration not allowed here");
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
            allowed = bucket->iassert(level > 0, statement->origin, "expected declaration");
            [[fallthrough]];
        default:
            break;
    }

    if (!allowed)
        return false;

    switch (statement->statement_type) {
        case ast::SCOPE_STMT:
            return verify_scope((ast::ScopeStatement *) statement);
        case ast::FUNC_STMT:
            return verify_function((ast::FuncStatement *) statement);
        case ast::FUNC_DECL_STMT:
            return verify_func_decl((ast::FuncDeclareStatement *) statement);
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
        case ast::VARIABLE_STMT:
            return verify_variable((ast::VariableStatement *) statement).res;
        case ast::STRUCT_STMT:
            return verify_struct((ast::StructStatement *) statement);
        case ast::IMPORT_STMT:
            return verify_import((ast::ImportStatement *) statement);
        case ast::EXPR_STMT:
            return verify_expression((ast::Expression *) statement);
    }
    return true;
}

bool Analyser::verify_statements(const std::vector<ast::Statement *> &statements) {
    bool res = true;

    for (auto statement : statements) {
        // Combining these two lines will cause clang to not call `verify_statement` when res is false, effectively limiting us to a single error
        bool t = verify_statement(statement);
        res = res and t;
    }
    return res;
}

bool Analyser::verify_scope(ast::ScopeStatement *scope, std::string name) {
    size_t old_var_count = variables.size();

    for (auto var : variables) {
        var->push_state(*var->state());
    }

    path.push_back(name);

    level++;

    bool res = verify_statements(scope->block);

    level--;

    while (old_var_count < variables.size())
        variables.pop_back();

    // todo: for loops this doesn't catch all cases where defines at the start immediately follow defines at the end
    for (auto var : variables) {
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

    path.pop_back();

    return res;
}

bool Analyser::verify_function(ast::FuncStatement *func) {
    variables.clear();
    last_loop = nullptr; // this shouldn't do anything, but just to be sure

    std::vector<std::string> func_path;
    if (func->member_of.has_value()) {
        func_path = get_global_path(func->member_of.value().get_path());
        func_path.push_back(func->name.raw);
    } else {
        func_path = get_global_path(func->name.raw);
    }
    bool res = true;

    for (auto [path, registered] : functions) {
        if (path != func_path)
            continue;
        bucket->error(func->name.origin, "redefinition of '{}'", func->name.raw);
        bucket->note(registered->name.origin, "previous definition here");
        res = false;
    }

    UPDATE_RES(verify_type(&func->return_type))
    if (func->member_of.has_value())
        UPDATE_RES(verify_type(&func->member_of.value()))

    UPDATE_RES(bucket->iassert(func->return_type == Type(VOID) || does_always_return(func),
        func->origin,
        "function with return type doesn't always return"))
    return_type = func->return_type;
    functions.emplace(func_path, func);
    for (auto arg : func->arguments) {
        auto sem = verify_variable(arg);

        if (sem.var)
            sem.var->state()->make_definitely_defined(arg->origin);
        UPDATE_RES(sem.res)
    }

    if (func->var_arg)
        bucket->warning(func->origin, "function uses var args, but they cannot be accessed");

    func->name.raw = flatten_path(func_path);

    UPDATE_RES(verify_scope(func, func->name.raw))
    return res;
}

bool Analyser::verify_func_decl(ast::FuncDeclareStatement *decl) {
    return true;
}

bool Analyser::verify_if(ast::IfStatement *if_) {
    if (if_->condition->type != Type(BOOL, 0)) {
        if_->condition = new ast::BinaryOperatorExpression(if_->condition->origin,
                                                      ast::NEQ,
                                                      if_->condition,
                                                      new ast::IntExpression(if_->condition->origin, "0"));
    }

    bool res = true;
    UPDATE_RES(verify_expression(if_->condition))
    UPDATE_RES(verify_scope(if_))
    if (if_->else_statement)
        UPDATE_RES(verify_scope(if_->else_statement))

    return res;
}

bool Analyser::verify_else(ast::ElseStatement *else_) {
    bucket->error(else_->origin, "else, but no preceding if");
    return false;
}

bool Analyser::verify_return(ast::ReturnStatement *return_) {
    bool res = true;
    if (return_->value) {
        UPDATE_RES(verify_expression(return_->value))
        if (res)
            UPDATE_RES(bucket->iassert(return_type.is_compatible(return_->value->type),
            return_->value->origin,
            "can't return value of type '{}' in function with return type '{}'",
            return_->value->type.str(),
            return_type.str()))
    } else
        res = bucket->iassert(return_type == Type(VOID),
                              return_->origin,
                              "function with return type should return a value");
    return res;
}

bool Analyser::verify_while(ast::WhileStatement *while_) {
    ast::Statement *old_last_loop = last_loop;
    last_loop = while_;

    if (while_->condition->type != Type(BOOL, 0)) {
        while_->condition = new ast::BinaryOperatorExpression(while_->condition->origin,
                                                         ast::NEQ,
                                                         while_->condition,
                                                         new ast::IntExpression(while_->condition->origin, "0"));
    }

    bool res = true;
    UPDATE_RES(verify_expression(while_->condition))
    UPDATE_RES(verify_scope(while_))
    last_loop = old_last_loop;
    return res;
}

bool Analyser::verify_break(ast::BreakStatement *break_) {
    return bucket->iassert(last_loop, break_->origin, "break outside of loop");
}

bool Analyser::verify_continue(ast::ContinueStatement *continue_) {
    return bucket->iassert(last_loop, continue_->origin, "continue outside of loop");
}

Analyser::VariableVerificationResult Analyser::verify_variable(ast::VariableStatement *var) {
    bool valid_type = verify_type(&var->type);

    bool res = true;

    for (auto variable : variables) {
        if (var->name.raw == variable->var->name.raw) {
            bucket->error(var->name.origin, "redefinition of '{}'", var->name.raw);
            bucket->note(variable->var->name.origin, "previous definition here");
            res = false;
        }
    }

    if (!valid_type)
        return {nullptr, false};

    SemanticVariable *sem;
    if (var->type.is_primitive()) {
        sem = new PrimitiveVariable(var);
    } else {
        ast::StructStatement *st = get_struct(var->type.get_user());

        std::vector<SemanticVariable *> member_states;
        for (auto member : st->members) {
            auto *temp = new ast::VariableStatement(var->origin,
                                               member->type,
                                               Token::name(var->name.raw + "." + member->name.raw));

            auto semantic_member = verify_variable(temp);
            if (semantic_member.var)
                member_states.push_back(semantic_member.var);
            UPDATE_RES(semantic_member.res)
        }

        sem = new CompoundVariable(var, member_states);
    }
    variables.push_back(sem);
    return {sem, res};
}

bool Analyser::verify_struct(ast::StructStatement *struct_) {
    std::vector<std::string> registered;
    std::vector<std::string> struct_path = get_global_path(struct_->name.raw);

    bool res = true;

    for (auto [path, registered] : structures) {
        if (path != struct_path)
            continue;
        bucket->error(struct_->name.origin, "redefinition of '{}'", struct_->name.raw);
        bucket->note(registered->name.origin, "previous definition here");
        res = false;
    }

    std::vector<ast::VariableStatement *> ctor_args;
    std::vector<ast::Statement *> body;

    auto *instance = new ast::VariableStatement(struct_->origin, struct_->get_type(path), Token::name("_instance"));
    body.push_back(instance);

    for (auto member : struct_->members) {
        UPDATE_RES(bucket->iassert(
            std::find(registered.begin(), registered.end(), member->name.raw) == registered.end(),
            member->name.origin,
            "duplicate member '{}'",
            member->name.raw))

        UPDATE_RES(verify_type(&member->type))

        registered.push_back(member->name.raw);
        ctor_args.push_back(new ast::VariableStatement(struct_->origin, member->type, member->name));

        auto *member_access = new ast::BinaryOperatorExpression(struct_->origin,
                                                           ast::MEM_ACC,
                                                           new ast::NameExpression(struct_->origin, "_instance"),
                                                           new ast::NameExpression(struct_->origin, member->name.raw));

        body.push_back(new ast::BinaryOperatorExpression(struct_->origin,
                                                    ast::ASSIGN,
                                                    member_access,
                                                    new ast::NameExpression(struct_->origin, member->name.raw)));
    }
    body.push_back(new ast::ReturnStatement(struct_->origin, new ast::NameExpression(struct_->origin, "_instance")));

    auto *ctor = new ast::FuncStatement(struct_->origin,
                                   Token::name(struct_->name.raw + "::$constructor", struct_->origin),
                                   struct_->get_type(path),
                                   ctor_args,
                                   body,
                                   false,
                                   {}); // fixme: this seems off

    structures.emplace(struct_path, struct_);

    // fixme: this throws additional errors if member types are invalid
    UPDATE_RES(verify_function(ctor))

    struct_path.push_back("$constructor");

    declarations.emplace(struct_path,
                         new ast::FuncDeclareStatement(ctor->origin,
                                                  ctor->name,
                                                  ctor->return_type,
                                                  ctor->arguments,
                                                  ctor->var_arg,
                                                  ctor->member_of));

    struct_->name.raw = flatten_path(struct_->name.raw);

    return res;
}

bool Analyser::verify_import(ast::ImportStatement *import_) {
    path.push_back(import_->name);
    bool res = verify_statements(import_->block);
    path.pop_back();

    return res;
}

bool Analyser::verify_expression(ast::Expression *expression, bool assigned_to, bool member_acc) {
    bool res = true;

    switch (expression->expression_type) {
        case ast::CALL_EXPR: {
            auto ce = (ast::CallExpression *) expression;

            std::vector<std::string> func_path;
            if (auto *gl = (ast::PrefixOperatorExpression *) ce->callee;
                ce->callee->expression_type == ast::NAME_EXPR || ce->callee->expression_type == ast::PATH_EXPR ||
                (ce->callee->expression_type == ast::PREFIX_EXPR && gl->prefix_type == ast::GLOBAL)) {
                func_path = ::flatten_path(ce->callee);

                if (is_struct_declared(func_path)) {
                    func_path.push_back("$constructor");
                    // TODO: check if there exists a function with the same name
                }
            } else if (ce->callee->expression_type == ast::MEM_ACC_EXPR) {
                auto *mem = (ast::BinaryOperatorExpression *) ce->callee;
                verify_expression(mem->left);

                if (mem->right->expression_type != ast::NAME_EXPR) {
                    bucket->error(mem->right->origin, "expected function name");
                    return false;
                }

                func_path = mem->left->type.get_path();
                func_path.push_back(((ast::NameExpression *) mem->right)->name);

                ce->arguments.insert(ce->arguments.begin(), mem->left);
                mem->left = nullptr;
            } else {
                bucket->error(ce->callee->origin, "calling of expressions is unimplemented");
                return false;
            }

            auto *name = new ast::NameExpression(ce->callee->origin, flatten_path(func_path));
            delete ce->callee;
            ce->callee = name;

            if (!bucket->iassert(is_func_declared(func_path),
                                 ce->callee->origin,
                                 "undefined function '{}'",
                                 flatten_path(func_path)))
                return false;
            ast::FuncStCommon *func = get_func_decl(func_path);

            ce->assign_type(func->return_type);

            size_t i = 0;
            if (func->member_of.has_value()) {
                if (func->arguments.size() > 0 && func->arguments[0]->name.raw == "this")
                    i = 1;
                else
                    // function is static, but we always put in the this argument
                    ce->arguments.erase(ce->arguments.begin());
            }

            if (!bucket->iassert(ce->arguments.size() >= func->arguments.size(),
                                 ce->origin,
                                 "too few arguments, expected {} found {}.",
                                 func->arguments.size(),
                                 ce->arguments.size()) || !bucket->iassert(
                func->var_arg || ce->arguments.size() <= func->arguments.
                                                               size(),
                ce->origin,
                "too many arguments, expected {} found {}.",
                func->arguments.size(),
                ce->arguments.size()))
                res = false;

            for (; i < std::min(func->arguments.size(), ce->arguments.size()); i++) {
                ast::VariableStatement *arg_var = func->arguments[i];
                ast::Expression *arg = ce->arguments[i];
                if (!verify_expression(arg) || !bucket->iassert(arg_var->type.is_compatible(arg->type),
                                                                arg->origin,
                                                                "passing value of type '{}' to argument of type '{}'",
                                                                arg->type.str(),
                                                                arg_var->type.str()) || !verify_expression(arg))
                    res = false;
            }
            break;
        }
        case ast::DASH_EXPR:
        case ast::DOT_EXPR:
        case ast::EQ_EXPR:
        case ast::COMP_EXPR:
        case ast::ASSIGN_EXPR: {
            auto ae = (ast::BinaryOperatorExpression *) expression;

            UPDATE_RES(verify_expression(ae->left, expression->expression_type == ast::ASSIGN_EXPR))
            UPDATE_RES(verify_expression(ae->right))
            if (res)
                res = bucket->iassert(ae->left->type.is_compatible(ae->right->type),
                                      ae->right->origin,
                                      "can't assign to type '{}' from '{}'",
                                      ae->left->type.str(),
                                      ae->right->type.str());

            if (expression->expression_type == ast::EQ_EXPR || expression->expression_type == ast::COMP_EXPR)
                ae->assign_type(Type(BOOL));
                // Todo: this is wrong; we generate LLVM code that casts to float if either operand is a float and
                //  to the highest bit width of the two operand
            else
                ae->assign_type(ae->left->type);
            break;
        }
        case ast::MEM_ACC_EXPR: {
            auto mae = (ast::BinaryOperatorExpression *) expression;
            auto left = mae->left;
            auto right = mae->right;

            UPDATE_RES(verify_expression(left, assigned_to, true))

            if (!bucket->iassert(!left->type.is_primitive(),
                                 left->origin,
                                 "'{}' is not a structure",
                                 left->type.str()))
                return false;
            auto struct_name = left->type.get_user();
            if (!bucket->iassert(is_struct_declared({struct_name}),
                                 left->origin,
                                 "undefined structure '{}'",
                                 left->type.str()))
                return false;
            ast::StructStatement *s = get_struct({struct_name});

            if (!bucket->iassert(right->expression_type == ast::NAME_EXPR, right->origin, "expected identifier"))
                return false;

            std::string member_name = ((ast::NameExpression *) right)->name;

            if (!bucket->iassert(s->has_member(member_name),
                                 right->origin,
                                 "no member named '{}' in '{}'",
                                 member_name,
                                 left->type.str()))
                return false;

            mae->assign_type(s->get_member_type(member_name));

            if (left->flattens_to_member_access()) {
                std::string name = left->flatten_to_member_access() + "." + member_name;
                UPDATE_RES(verify_expression(new ast::NameExpression(mae->origin, name), assigned_to))
            }

            break;
        }
        case ast::PREFIX_EXPR: {
            auto pe = (ast::PrefixOperatorExpression *) expression;
            UPDATE_RES(verify_expression(pe->operand))

            Type pe_type = pe->operand->type;

            switch (pe->prefix_type) {
                case ast::NEG:
                case ast::LOG_NOT:
                    UPDATE_RES(bucket->iassert(pe_type.is_primitive() || pe_type.pointer_level > 0,
                        pe->operand->origin,
                        "invalid operand to unary expression"))
                break;
                case ast::REF: {
                    pe_type.pointer_level++;
                    ast::ExprType etype = pe->operand->expression_type;

                    if (!bucket->iassert(etype == ast::NAME_EXPR || etype == ast::MEM_ACC_EXPR,
                                         pe->operand->origin,
                                         "cannot take reference of temporary value")) {
                        res = false;
                        bucket->note(pe->operand->origin,
                                     "'{}' produces a temporary value",
                                     pe->operand->print());
                    }
                    break;
                }
                case ast::DEREF:
                    UPDATE_RES(bucket->iassert(pe_type.pointer_level > 0,
                        pe->operand->origin,
                        "cannot dereference non-pointer type '{}'",
                        pe->operand->type.str()))
                    pe_type.pointer_level--;
                    break;
                case ast::GLOBAL:
                    res = false;
                    bucket->error(pe->origin, "internal: unhandled global path prefix");
                    break;
            }

            pe->assign_type(pe_type);
            break;
        }
        case ast::NAME_EXPR: {
            auto ne = (ast::NameExpression *) expression;
            if (!bucket->iassert(is_var_declared(ne->name),
                                 expression->origin,
                                 "undefined variable '{}'",
                                 ne->name))
                return false;

            SemanticVariable *var = get_variable(ne->name);

            ne->assign_type(var->var->type);

            // don't do variable state checking if we are in a (pure) member access
            if (member_acc)
                break;

            if (assigned_to) {
                if (var->state()->is_definitely_defined()) {
                    bucket->warning(var->state()->get_defined_pos(), "assigned value is immediately discarded");
                    bucket->note(expression->origin, "overwritten here");
                } else if (var->state()->is_maybe_defined()) {
                    bucket->warning(expression->origin,
                                    "value might overwrite previous assignment without reading it");
                }

                var->state()->make_definitely_defined(expression->origin);
            } else {
                if (!bucket->iassert(!var->state()->is_definitely_undefined(),
                                     expression->origin,
                                     "value is still undefined"))
                    res = false;
                else if (!bucket->iassert(!var->state()->is_maybe_undefined(),
                                          expression->origin,
                                          "value might be undefined"))
                    res = false;

                var->state()->make_definitely_read(expression->origin);
            }
            break;
        }
        case ast::INT_EXPR:
            expression->assign_type(Type(I32));
            break;
        case ast::REAL_EXPR:
            expression->assign_type(Type(F32));
            break;
        case ast::STR_EXPR:
            expression->assign_type(Type(U8, 1));
            break;
        case ast::BOOL_EXPR:
            expression->assign_type(Type(BOOL));
            break;
        default:
            // todo: do more analysis here:
            //  if it's a path, put an error after the expression: if the path points to a valid function, suggest
            //  calling it. if it points to a valid struct, suggest creating a variable with it
            bucket->error(expression->origin, "internal: unexpected, unhandled type of expression");
            res = false;
    }
    return res;
}

bool Analyser::verify_type(Type *type) {
    bool res = true;

    if (!type->is_primitive()) {
        auto path = type->get_user();
        if (path.front().empty()) {
            path.erase(path.begin());
        } else {
            // global_path here, means global path to local struct
            // precisely, type->get_user() contains a local path
            // and global_path contains a global_path, both to the
            // same type. if type->get_user() interpreted locally
            // points to a type it is converted to a global path
            // and used, otherwise it is left as is and interpreted
            // as a global path
            auto global_path = get_global_path(path);
            if (is_struct_declared(global_path)) {
                path = global_path;
            }
        }

        res = bucket->iassert(is_struct_declared(path),
                              type->origin,
                              "undefied type '{}'",
                              flatten_path(path));

        type->set_user(path);
    }

    return res;
}

bool Analyser::does_always_return(ast::ScopeStatement *scope) {
    for (auto &it : scope->block) {
        if (it->statement_type == ast::RETURN_STMT || (
                it->statement_type == ast::SCOPE_STMT && does_always_return((ast::ScopeStatement *) it)) || (
                it->statement_type == ast::IF_STMT && ((ast::IfStatement *) it)->else_statement &&
                does_always_return((ast::ScopeStatement *) it) && does_always_return(((ast::IfStatement *) it)->else_statement))
            || (it->statement_type == ast::WHILE_STMT && does_always_return((ast::ScopeStatement *) it)))
            return true;
    }
    return false;
}

bool Analyser::is_var_declared(const std::string &name) {
    return std::find_if(variables.begin(),
                        variables.end(),
                        [name](SemanticVariable *v) {
                            return name == v->var->name.raw;
                        }) != variables.end();
}

bool Analyser::is_func_declared(const std::vector<std::string> &name) {
    auto global = name;
    if (name.front().empty()) {
        global.erase(global.begin());
        return declarations.contains(global);
    }

    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    return declarations.contains(global) || declarations.contains(local);
}

bool Analyser::is_struct_declared(const std::vector<std::string> &name) {
    return structures.contains(name);
}

SemanticVariable *Analyser::get_variable(const std::string &name) {
    return *std::find_if(variables.begin(),
                         variables.end(),
                         [name](SemanticVariable *v) {
                             return name == v->var->name.raw;
                         });
}

ast::FuncStCommon *Analyser::get_func_decl(const std::vector<std::string> &name) {
    auto global = name;
    if (name.front().empty()) {
        global.erase(global.begin());
        return declarations[global];
    }

    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    if (declarations.contains(local))
        return declarations[local];

    return declarations[global];
}

ast::StructStatement *Analyser::get_struct(const std::vector<std::string> &name) {
    auto global = name;
    if (name.front().empty()) {
        global.erase(global.begin());
        return structures[global];
    }

    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    if (structures.contains(local))
        return structures[local];

    return structures[global];
}

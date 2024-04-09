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

std::vector<std::string> Analyser::get_local_path(const std::string &name) const {
    auto local = path;
    local.push_back(name);
    return local;
}

std::vector<std::string> Analyser::get_local_path(const std::vector<std::string> &name) const {
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
    std::vector<Statement *> body;
    body.push_back(new ReturnStatement(lr, new CallExpression(lr, new NameExpression(lr, "::main"), {})));

    auto *func = new FuncStatement(lr, Token::name("main"), Type(U32), {}, body, false);
    auto *decl = new FuncDeclareStatement(lr, Token::name("main"), Type(I32), {}, false);

    functions.emplace(std::vector<std::string>(), func);
    declarations.emplace(std::vector<std::string>(), decl);
}

bool Analyser::verify_statement(Statement *statement) {
    bool allowed = true;

    switch (statement->statement_type) {
        case STRUCT_STMT:
        case IMPORT_STMT:
        case FUNC_STMT:
            allowed = bucket->iassert(level == 0, statement->origin, "declaration not allowed here");
            break;
        case IF_STMT:
        case ELSE_STMT:
        case RETURN_STMT:
        case WHILE_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
        case VARIABLE_STMT:
        case EXPR_STMT:
        case SCOPE_STMT:
            allowed = bucket->iassert(level > 0, statement->origin, "expected declaration");
            [[fallthrough]];
        default:
            break;
    }

    if (!allowed)
        return false;

    switch (statement->statement_type) {
        case SCOPE_STMT:
            return verify_scope((ScopeStatement *) statement);
        case FUNC_STMT:
            return verify_function((FuncStatement *) statement);
        case FUNC_DECL_STMT:
            return verify_func_decl((FuncDeclareStatement *) statement);
        case IF_STMT:
            return verify_if((IfStatement *) statement);
        case ELSE_STMT:
            return verify_else((ElseStatement *) statement);
        case RETURN_STMT:
            return verify_return((ReturnStatement *) statement);
        case WHILE_STMT:
            return verify_while((WhileStatement *) statement);
        case BREAK_STMT:
            return verify_break((BreakStatement *) statement);
        case CONTINUE_STMT:
            return verify_continue((ContinueStatement *) statement);
        case VARIABLE_STMT:
            return verify_variable((VariableStatement *) statement);
        case STRUCT_STMT:
            return verify_struct((StructStatement *) statement);
        case IMPORT_STMT:
            return verify_import((ImportStatement *) statement);
        case EXPR_STMT:
            return verify_expression((Expression *) statement);
    }
    return true;
}

bool Analyser::verify_statements(const std::vector<Statement *> &statements) {
    bool res = true;

    for (auto statement : statements) {
        res = res && bucket->iassert(statement->statement_type != FUNC_DECL_STMT,
                                     statement->origin,
                                     "internal: premature function declaration: report this as a bug");
        if (statement->statement_type == FUNC_STMT) {
            auto func = reinterpret_cast<FuncStatement *>(statement);
            std::vector<std::string> name = get_local_path(func->name.raw);
            declarations.emplace(name,
                                 new FuncDeclareStatement(statement->origin,
                                                          Token::name(flatten_path(name), func->name.origin),
                                                          func->return_type,
                                                          func->arguments,
                                                          func->var_arg));
        }
    }

    for (auto statement : statements) {
        // Combining these two lines will cause clang to not call `verify_statement` when res is false, effectively limiting us to a single error
        bool t = verify_statement(statement);
        res = res and t;
    }
    return res;
}

std::vector<Statement *> Analyser::finish() {
    std::vector<Statement *> res;
    res.reserve(structures.size() + declarations.size() + functions.size());
    for (auto [_, st] : structures)
        res.push_back(st);
    for (auto [_, dc] : declarations)
        res.push_back(dc);
    for (auto [_, fn] : functions)
        res.push_back(fn);
    return res;
}

bool Analyser::verify_scope(ScopeStatement *scope, std::string name) {
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

        if (scope->statement_type == SCOPE_STMT) {
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

bool Analyser::verify_function(FuncStatement *func) {
    variables.clear();
    last_loop = nullptr; // this shouldn't do anything, but just to be sure

    std::vector<std::string> func_path = get_local_path(func->name.raw);

    for (auto [path, registered] : functions) {
        if (path != func_path)
            continue;
        bucket->error(func->name.origin, "redefinition of '{}'", func->name.raw);
        bucket->note(registered->name.origin, "previous definition here");
        return false;
    }

    if (!bucket->iassert(func->return_type == Type(VOID) || does_always_return(func),
                         func->origin,
                         "function with return type doesn't always return"))
        return false;
    return_type = func->return_type;
    functions.emplace(func_path, func);
    for (auto arg : func->arguments) {
        verify_variable(arg)->state()->make_definitely_defined(arg->origin);
    }

    if (func->var_arg)
        bucket->warning(func->origin, "function uses var args, but they cannot be accessed");

    func->name.raw = flatten_path(func->name.raw);

    return verify_scope(func, func->name.raw);
}

bool Analyser::verify_func_decl(FuncDeclareStatement *decl) {
    return true;
}

bool Analyser::verify_if(IfStatement *if_) {
    if (if_->condition->type != Type(BOOL, 0)) {
        if_->condition = new BinaryOperatorExpression(if_->condition->origin,
                                                      NEQ,
                                                      if_->condition,
                                                      new IntExpression(if_->condition->origin, "0"));
    }

    return verify_expression(if_->condition) && verify_scope(if_) && (
        !if_->else_statement || verify_scope(if_->else_statement));
}

bool Analyser::verify_else(ElseStatement *else_) {
    bucket->error(else_->origin, "else, but no preceding if");
    return false;
}

bool Analyser::verify_return(ReturnStatement *return_) {
    if (return_->value)
        return verify_expression(return_->value) && bucket->iassert(return_type.is_compatible(return_->value->type),
                                                                    return_->value->origin,
                                                                    "can't return value of type '{}' in function with return type '{}'",
                                                                    return_->value->type.str(),
                                                                    return_type.str());
    else
        return bucket->iassert(return_type == Type(VOID),
                               return_->origin,
                               "function with return type should return a value");
}

bool Analyser::verify_while(WhileStatement *while_) {
    Statement *old_last_loop = last_loop;
    last_loop = while_;

    if (while_->condition->type != Type(BOOL, 0)) {
        while_->condition = new BinaryOperatorExpression(while_->condition->origin,
                                                         NEQ,
                                                         while_->condition,
                                                         new IntExpression(while_->condition->origin, "0"));
    }

    bool res = verify_expression(while_->condition) && verify_scope(while_);
    last_loop = old_last_loop;
    return res;
}

bool Analyser::verify_break(BreakStatement *break_) {
    return bucket->iassert(last_loop, break_->origin, "break outside of loop");
}

bool Analyser::verify_continue(ContinueStatement *continue_) {
    return bucket->iassert(last_loop, continue_->origin, "continue outside of loop");
}

SemanticVariable *Analyser::verify_variable(VariableStatement *var) {
    if (!var->type.is_primitive() && !bucket->iassert(is_struct_declared(var->type.get_user()),
                                                      var->origin,
                                                      "undefied type '{}'",
                                                      flatten_path(var->type.get_user())))
        return nullptr;

    for (auto variable : variables) {
        if (var->name.raw == variable->var->name.raw) {
            bucket->error(var->name.origin, "redefinition of '{}'", var->name.raw);
            bucket->note(variable->var->name.origin, "previous definition here");
            return nullptr;
        }
    }

    SemanticVariable *res;
    if (var->type.is_primitive()) {
        res = new PrimitiveVariable(var);
    } else {
        StructStatement *st = get_struct(var->type.get_user());

        std::vector<SemanticVariable *> member_states;
        for (auto member : st->members) {
            auto *temp = new VariableStatement(var->origin,
                                               member->type,
                                               Token::name(var->name.raw + "." + member->name.raw));

            member_states.push_back(verify_variable(temp));
        }

        res = new CompoundVariable(var, member_states);
    }
    variables.push_back(res);
    return res;
}

bool Analyser::verify_struct(StructStatement *struct_) {
    std::vector<std::string> registered;
    std::vector<std::string> struct_path = get_local_path(struct_->name.raw);

    for (auto [path, registered] : structures) {
        if (path != struct_path)
            continue;
        bucket->error(struct_->name.origin, "redefinition of '{}'", struct_->name.raw);
        bucket->note(registered->name.origin, "previous definition here");
        return false;
    }

    std::vector<VariableStatement *> ctor_args;
    std::vector<Statement *> body;

    auto *instance = new VariableStatement(struct_->origin, struct_->get_type(path), Token::name("_instance"));
    body.push_back(instance);

    for (auto member : struct_->members) {
        if (!bucket->iassert(std::find(registered.begin(), registered.end(), member->name.raw) == registered.end(),
                             member->name.origin,
                             "duplicate member '{}'",
                             member->name.raw))
            return false;
        registered.push_back(member->name.raw);
        ctor_args.push_back(new VariableStatement(struct_->origin, member->type, member->name));

        auto *member_access = new BinaryOperatorExpression(struct_->origin,
                                                           MEM_ACC,
                                                           new NameExpression(struct_->origin, "_instance"),
                                                           new NameExpression(struct_->origin, member->name.raw));

        body.push_back(new BinaryOperatorExpression(struct_->origin,
                                                    ASSIGN,
                                                    member_access,
                                                    new NameExpression(struct_->origin, member->name.raw)));
    }
    body.push_back(new ReturnStatement(struct_->origin, new NameExpression(struct_->origin, "_instance")));

    auto *ctor = new FuncStatement(struct_->origin,
                                   Token::name(struct_->name.raw + "::$constructor", struct_->origin),
                                   struct_->get_type(path),
                                   ctor_args,
                                   body,
                                   false);

    structures.emplace(struct_path, struct_);

    bool res = verify_function(ctor);

    struct_path.push_back("$constructor");

    declarations.emplace(struct_path,
                         new FuncDeclareStatement(ctor->origin,
                                                  ctor->name,
                                                  ctor->return_type,
                                                  ctor->arguments,
                                                  ctor->var_arg));

    struct_->name.raw = flatten_path(struct_->name.raw);

    return res;
}

bool Analyser::verify_import(ImportStatement *import_) {
    Analyser import_analyser(bucket, __no_auto_main {});
    const bool res = import_analyser.verify_statements(import_->block);

    path.push_back(import_->name);

    for (auto [local_path, struct_] : import_analyser.structures) {
        auto global_path = get_local_path(local_path);
        struct_->name.raw = flatten_path(global_path);
        structures.emplace(global_path, struct_);
    }

    for (auto [local_path, decl] : import_analyser.declarations) {
        auto global_path = get_local_path(local_path);
        decl->name.raw = flatten_path(global_path);
        declarations.emplace(global_path, decl);
    }

    for (auto [local_path, func] : import_analyser.functions) {
        auto global_path = get_local_path(local_path);
        func->name.raw = flatten_path(global_path);
        functions.emplace(global_path, func);
    }

    path.pop_back();

    return res;
}

bool Analyser::verify_expression(Expression *expression, bool assigned_to, bool member_acc) {
    switch (expression->expression_type) {
        case CALL_EXPR: {
            auto ce = (CallExpression *) expression;

            std::vector<std::string> func_path;
            if (auto *gl = (PrefixOperatorExpression *) ce->callee;
                ce->callee->expression_type == NAME_EXPR || ce->callee->expression_type == PATH_EXPR ||
                (ce->callee->expression_type == PREFIX_EXPR && gl->prefix_type == GLOBAL)) {
                func_path = ::flatten_path(ce->callee);

                if (is_struct_declared(func_path)) {
                    func_path.push_back("$constructor");
                    // TODO: check if there exists a function with the same name
                }

                if (ce->callee->expression_type == NAME_EXPR) {
                    ((NameExpression *) ce->callee)->name = flatten_path(func_path);
                } else {
                    auto *name = new NameExpression(ce->callee->origin, flatten_path(func_path));
                    delete ce->callee;
                    ce->callee = name;
                }
            } else {
                return bucket->iassert(false, ce->callee->origin, "calling of expressions is unimplemented");
            }

            if (!bucket->iassert(is_func_declared(func_path),
                                 ce->callee->origin,
                                 "undefined function '{}'",
                                 flatten_path(func_path)))
                return false;
            FuncStCommon *func = get_func_decl(func_path);
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
                return false;

            for (size_t i = 0; i < func->arguments.size(); i++) {
                VariableStatement *arg_var = func->arguments[i];
                Expression *arg = ce->arguments[i];
                if (!verify_expression(arg) || !bucket->iassert(arg_var->type.is_compatible(arg->type),
                                                                arg->origin,
                                                                "passing value of type '{}' to argument of type '{}'",
                                                                arg->type.str(),
                                                                arg_var->type.str()) || !verify_expression(arg))
                    return false;
            }
            ce->assign_type(func->return_type);
            break;
        }
        case DASH_EXPR:
        case DOT_EXPR:
        case EQ_EXPR:
        case COMP_EXPR:
        case ASSIGN_EXPR: {
            auto ae = (BinaryOperatorExpression *) expression;
            if (!verify_expression(ae->left, expression->expression_type == ASSIGN_EXPR) || !
                verify_expression(ae->right) || !bucket->iassert(ae->left->type.
                                                                     is_compatible(ae->right->type),
                                                                 ae->right->origin,
                                                                 "can't assign to type '{}' from '{}'",
                                                                 ae->left->type.str(),
                                                                 ae->right->type.str()))
                return false;
            if (expression->expression_type == EQ_EXPR || expression->expression_type == COMP_EXPR)
                ae->assign_type(Type(BOOL));
                // Todo: this is wrong; we generate LLVM code that casts to float if either operand is a float and
                //  to the highest bit width of the two operand
            else
                ae->assign_type(ae->left->type);
            break;
        }
        case MEM_ACC_EXPR: {
            auto mae = (BinaryOperatorExpression *) expression;
            auto left = mae->left;
            auto right = mae->right;

            if (!verify_expression(left, assigned_to, true))
                return false;

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
            StructStatement *s = get_struct({struct_name});
            if (!bucket->iassert(right->expression_type == NAME_EXPR, right->origin, "expected identifier"))
                return false;
            std::string member_name = ((NameExpression *) right)->name;

            if (!bucket->iassert(s->has_member(member_name),
                                 right->origin,
                                 "no member named '{}' in '{}'",
                                 member_name,
                                 left->type.str()))
                return false;
            mae->assign_type(s->get_member_type(member_name));

            if (left->flattens_to_member_access()) {
                std::string name = left->flatten_to_member_access() + "." + member_name;
                return verify_expression(new NameExpression(mae->origin, name), assigned_to);
            }

            break;
        }
        case PREFIX_EXPR: {
            auto pe = (PrefixOperatorExpression *) expression;
            if (!verify_expression(pe->operand))
                return false;

            bool res = true;
            Type pe_type = pe->operand->type;

            switch (pe->prefix_type) {
                case NEG:
                case LOG_NOT:
                    res = bucket->iassert(pe_type.is_primitive() || pe_type.pointer_level > 0,
                                          pe->operand->origin,
                                          "invalid operand to unary expression");
                    break;
                case REF: {
                    pe_type.pointer_level++;
                    ExprType etype = pe->operand->expression_type;
                    res = bucket->iassert(etype == NAME_EXPR || etype == MEM_ACC_EXPR,
                                          pe->operand->origin,
                                          "cannot take reference of temporary value");
                    if (!res)
                        bucket->note(pe->operand->origin,
                                     "'{}' produces a temporary value",
                                     pe->operand->print());
                    break;
                }
                case DEREF:
                    res = bucket->iassert(pe_type.pointer_level > 0,
                                          pe->operand->origin,
                                          "cannot dereference non-pointer type '{}'",
                                          pe->operand->type.str());
                    pe_type.pointer_level--;
                    break;
                case GLOBAL:
                    res = false;
                    bucket->error(pe->origin, "internal: Unhandled global path prefix");
                    break;
            }

            pe->assign_type(pe_type);
            return res;
        }
        case NAME_EXPR: {
            auto ne = (NameExpression *) expression;
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
                    return false;

                if (!bucket->iassert(!var->state()->is_maybe_undefined(),
                                     expression->origin,
                                     "value might be undefined"))
                    return false;

                var->state()->make_definitely_read(expression->origin);
            }
            break;
        }
        case INT_EXPR:
            expression->assign_type(Type(I32));
            break;
        case REAL_EXPR:
            expression->assign_type(Type(F32));
            break;
        case STR_EXPR:
            expression->assign_type(Type(U8, 1));
            break;
        case BOOL_EXPR:
            expression->assign_type(Type(BOOL));
            break;
        case NULL_EXPR:
            expression->assign_type(Type(VOID, 1));
            break;
        default:
            bucket->error(expression->origin, "internal: unexpected, unhandled type of expression");
            return false;
    }
    return true;
}

bool Analyser::does_always_return(ScopeStatement *scope) {
    for (auto &it : scope->block) {
        if (it->statement_type == RETURN_STMT || (
                it->statement_type == SCOPE_STMT && does_always_return((ScopeStatement *) it)) || (
                it->statement_type == IF_STMT && ((IfStatement *) it)->else_statement &&
                does_always_return((ScopeStatement *) it) && does_always_return(((IfStatement *) it)->else_statement))
            || (it->statement_type == WHILE_STMT && does_always_return((ScopeStatement *) it)))
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
        return functions.contains(global);
    }

    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    return declarations.contains(global) || declarations.contains(local);
}

bool Analyser::is_struct_declared(const std::vector<std::string> &name) {
    auto global = name;
    if (name.front().empty()) {
        global.erase(global.begin());
        return structures.contains(global);
    }

    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    return structures.contains(global) || structures.contains(local);
}

SemanticVariable *Analyser::get_variable(const std::string &name) {
    return *std::find_if(variables.begin(),
                         variables.end(),
                         [name](SemanticVariable *v) {
                             return name == v->var->name.raw;
                         });
}

FuncStCommon *Analyser::get_func_decl(const std::vector<std::string> &name) {
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

StructStatement *Analyser::get_struct(const std::vector<std::string> &name) {
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

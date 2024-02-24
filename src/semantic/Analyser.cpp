// tarik (c) Nikolas Wipper 2021-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Analyser.h"

#include <algorithm>
#include <iostream>

#include "Path.h"
#include "util/Util.h"
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

Analyser::Analyser() {
    LexerPos lp;
    std::vector<Statement *> body;
    body.push_back(new ReturnStatement(lp, new CallExpression(lp, new NameExpression(lp, "::main"), {})));

    auto *func = new FuncStatement(lp, "main", Type(U32), {}, body, false);
    auto *decl = new FuncDeclareStatement(lp, "main", Type(I32), {}, false);

    functions.emplace(std::vector<std::string>(), func);
    declarations.emplace(std::vector<std::string>(), decl);
}

bool Analyser::verify_statement(Statement *statement) {
    bool allowed = true;

    switch (statement->statement_type) {
        case STRUCT_STMT:
        case IMPORT_STMT:
        case FUNC_STMT:
            allowed = iassert(level == 0, statement->origin, "declaration not allowed here");
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
            allowed = iassert(level > 0, statement->origin, "expected declaration");
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
        iassert(statement->statement_type != FUNC_DECL_STMT,
                statement->origin,
                "internal: premature function declaration: report this as a bug");
        if (statement->statement_type == FUNC_STMT) {
            auto func = reinterpret_cast<FuncStatement *>(statement);
            std::vector<std::string> name = get_local_path(func->name);
            declarations.emplace(name,
                                 new FuncDeclareStatement(statement->origin,
                                                          flatten_path(name),
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

    path.push_back(name);

    level++;

    bool res = verify_statements(scope->block);

    level--;

    while (old_var_count < variables.size())
        variables.pop_back();

    path.pop_back();

    return res;
}

bool Analyser::verify_function(FuncStatement *func) {
    variables.clear();
    last_loop = nullptr; // this shouldn't do anything, but just to be sure

    std::vector<std::string> func_path = get_local_path(func->name);

    for (auto [path, registered] : functions) {
        if (path != func_path)
            continue;
        error(func->origin, "redefinition of '%s'", func->name.c_str());
        note(registered->origin, "previous definition here");
        return false;
    }

    if (!iassert(func->return_type == Type(VOID) || does_always_return(func),
                 func->origin,
                 "function with return type doesn't always return"))
        return false;
    functions.emplace(func_path, func);
    for (auto arg : func->arguments) {
        variables.push_back(arg);
    }

    if (func->var_arg)
        warning(func->origin, "function uses var args, but they cannot be accessed");

    func->name = flatten_path(func->name);

    return verify_scope(func, func->name);
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
    error(else_->origin, "Else, but no preceding if");
    return false;
}

bool Analyser::verify_return(ReturnStatement *return_) {
    if (return_->value)
        return verify_expression(return_->value);
    return true;
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
    return iassert(last_loop, break_->origin, "break outside of loop");
}

bool Analyser::verify_continue(ContinueStatement *continue_) {
    return iassert(last_loop, continue_->origin, "continue outside of loop");
}

bool Analyser::verify_variable(VariableStatement *var) {
    if (!var->type.is_primitive() && !iassert(is_struct_declared(var->type.get_user()),
                                              var->origin,
                                              "undefied type '%s'",
                                              flatten_path(var->type.get_user()).c_str()))
        return false;

    for (auto variable : variables) {
        if (var->name == variable->name) {
            error(var->origin, "redefinition of '%s'", var->name.c_str());
            note(variable->origin, "previous definition here");
            return false;
        }
    }
    variables.push_back(var);
    return true;
}

bool Analyser::verify_struct(StructStatement *struct_) {
    std::vector<std::string> registered;
    std::vector<std::string> struct_path = get_local_path(struct_->name.raw);

    for (auto [path, registered] : structures) {
        if (path != struct_path)
            continue;
        error(struct_->name.where, "redefinition of '%s'", struct_->name.raw.c_str());
        note(registered->name.where, "previous definition here");
        return false;
    }

    std::vector<VariableStatement *> ctor_args;
    std::vector<Statement *> body;

    auto *instance = new VariableStatement(struct_->origin, struct_->get_type(path), "_instance");
    body.push_back(instance);

    for (auto member : struct_->members) {
        if (!iassert(std::find(registered.begin(), registered.end(), member->name) == registered.end(),
                     member->origin,
                     "duplicate member '%s'",
                     member->name.c_str()))
            return false;
        registered.push_back(member->name);
        ctor_args.push_back(new VariableStatement(struct_->origin, member->type, member->name));

        auto *member_access = new BinaryOperatorExpression(struct_->origin,
                                                           MEM_ACC,
                                                           new NameExpression(struct_->origin, "_instance"),
                                                           new NameExpression(struct_->origin, member->name));

        body.push_back(new BinaryOperatorExpression(struct_->origin,
                                                    ASSIGN,
                                                    member_access,
                                                    new NameExpression(struct_->origin, member->name)));
    }
    body.push_back(new ReturnStatement(struct_->origin, new NameExpression(struct_->origin, "_instance")));

    auto *ctor = new FuncStatement(struct_->origin,
                                   struct_->name.raw + "::$constructor",
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
    Analyser import_analyser(__no_auto_main {});
    const bool res = import_analyser.verify_statements(import_->block);

    path.push_back(import_->name);

    for (auto [local_path, struct_] : import_analyser.structures) {
        auto global_path = get_local_path(local_path);
        struct_->name.raw = flatten_path(global_path);
        structures.emplace(global_path, struct_);
    }

    for (auto [local_path, decl] : import_analyser.declarations) {
        auto global_path = get_local_path(local_path);
        decl->name = flatten_path(global_path);
        declarations.emplace(global_path, decl);
    }

    for (auto [local_path, func] : import_analyser.functions) {
        auto global_path = get_local_path(local_path);
        func->name = flatten_path(global_path);
        functions.emplace(global_path, func);
    }

    path.pop_back();

    return res;
}

bool Analyser::verify_expression(Expression *expression) {
    switch (expression->expression_type) {
        case CALL_EXPR: {
            auto ce = (CallExpression *) expression;

            std::vector<std::string> func_path;
            if (ce->callee->expression_type == NAME_EXPR || ce->callee->expression_type == PATH_EXPR) {
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
                return iassert(false, ce->callee->origin, "calling of expressions is unimplemented");
            }

            if (!iassert(is_func_declared(func_path),
                         ce->callee->origin,
                         "undefined function '%s'",
                         flatten_path(func_path).c_str()))
                return false;
            FuncStCommon *func = get_func_decl(func_path);
            if (!iassert(ce->arguments.size() >= func->arguments.size(),
                         ce->origin,
                         "too few arguments, expected %i found %i.",
                         func->arguments.size(),
                         ce->arguments.size()) || !iassert(func->var_arg || ce->arguments.size() <= func->arguments.
                                                           size(),
                                                           ce->origin,
                                                           "too many arguments, expected %i found %i.",
                                                           func->arguments.size(),
                                                           ce->arguments.size()))
                return false;

            for (size_t i = 0; i < func->arguments.size(); i++) {
                VariableStatement *arg_var = func->arguments[i];
                Expression *arg = ce->arguments[i];
                if (!verify_expression(arg) || !iassert(arg_var->type.is_compatible(arg->type),
                                                        arg->origin,
                                                        "passing value of type '%s' to argument of type '%s'",
                                                        arg->type.str().c_str(),
                                                        arg_var->type.str().c_str()) || !verify_expression(arg))
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
            if (!verify_expression(ae->left) || !verify_expression(ae->right) || !iassert(ae->left->type.
                    is_compatible(ae->right->type),
                    ae->right->origin,
                    "can't assign to type '%s' from '%s'",
                    ae->left->type.str().c_str(),
                    ae->right->type.str().c_str()))
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

            if (!verify_expression(left))
                return false;

            if (!iassert(!left->type.is_primitive(), left->origin, "'%s' is not a structure", left->type.str().c_str()))
                return false;
            auto struct_name = left->type.get_user();
            if (!iassert(is_struct_declared({struct_name}),
                         left->origin,
                         "undefined structure '%s'",
                         left->type.str().c_str()))
                return false;
            StructStatement *s = get_struct({struct_name});
            if (!iassert(right->expression_type == NAME_EXPR, right->origin, "expected identifier"))
                return false;
            std::string member_name = ((NameExpression *) right)->name;

            if (!iassert(s->has_member(member_name),
                         right->origin,
                         "no member named '%s' in '%s'",
                         member_name.c_str(),
                         left->type.str().c_str()))
                return false;
            mae->assign_type(s->get_member_type(member_name));
            break;
        }
        case PREFIX_EXPR: {
            auto pe = (PrefixOperatorExpression *) expression;
            if (!verify_expression(pe->operand))
                return false;

            bool res = true;
            Type pe_type = pe->operand->type;

            switch (pe->prefix_type) {
                case POS:
                case NEG:
                case LOG_NOT:
                    res = iassert(pe_type.is_primitive() || pe_type.pointer_level > 0,
                                  pe->operand->origin,
                                  "invalid operand to unary expression");
                    break;
                case REF: {
                    pe_type.pointer_level++;
                    ExprType etype = pe->operand->expression_type;
                    res = iassert(etype == NAME_EXPR || etype == MEM_ACC_EXPR,
                                  pe->operand->origin,
                                  "cannot take reference of temporary value");
                    if (!res)
                        note(pe->operand->origin, "'%s' produces a temporary value", pe->operand->print().c_str());
                    break;
                }
                case DEREF:
                    res = iassert(pe_type.pointer_level > 0,
                                  pe->operand->origin,
                                  "cannot dereference non-pointer type '%s'",
                                  pe->operand->type.str().c_str());
                    pe_type.pointer_level--;
            }

            pe->assign_type(pe_type);
            return res;
        }
        case NAME_EXPR: {
            auto ne = (NameExpression *) expression;
            if (!iassert(is_var_declared(ne->name), expression->origin, "undefined variable '%s'", ne->name.c_str()))
                return false;
            ne->assign_type(get_variable(ne->name)->type);
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
    return std::find_if(variables.begin(), variables.end(), [name](VariableStatement *v) { return name == v->name; }) !=
           variables.end();
}

bool Analyser::is_func_declared(const std::vector<std::string> &name) {
    auto global = name;
    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    return declarations.contains(global) || declarations.contains(local);
}

bool Analyser::is_struct_declared(const std::vector<std::string> &name) {
    auto global = name;
    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    return structures.contains(global) || structures.contains(local);
}

VariableStatement *Analyser::get_variable(const std::string &name) {
    return *std::find_if(variables.begin(), variables.end(), [name](VariableStatement *v) { return name == v->name; });
}

FuncStCommon *Analyser::get_func_decl(const std::vector<std::string> &name) {
    auto global = name;
    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    if (declarations.contains(local))
        return declarations[local];

    return declarations[global];
}

StructStatement *Analyser::get_struct(const std::vector<std::string> &name) {
    auto global = name;
    auto local = path;
    local.insert(local.end(), name.begin(), name.end());

    if (structures.contains(local))
        return structures[local];

    return structures[global];
}

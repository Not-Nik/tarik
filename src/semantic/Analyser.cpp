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
    : bucket(bucket) {}

std::vector<aast::Statement *> Analyser::finish() {
    std::vector<aast::Statement *> res;
    res.reserve(structures.size() + declarations.size() + functions.size());
    for (auto [_, st] : structures)
        res.push_back(st);
    for (auto [_, dc] : declarations)
        res.push_back(dc);
    for (auto fn : functions)
        res.push_back(fn);
    return res;
}

void Analyser::analyse(const std::vector<ast::Statement *> &statements) {
    analyse_import(statements);
    verify_statements(statements);
}

void Analyser::analyse_import(const std::vector<ast::Statement *> &statements) {
    for (auto statement : statements) {
        if (statement->statement_type == ast::FUNC_STMT) {
            auto func = reinterpret_cast<ast::FuncStatement *>(statement);
            Path name = Path({});
            if (func->member_of.has_value()) {
                name = func->member_of.value().get_path().with_prefix(path).create_member(func->name.raw);
            } else {
                name = Path({func->name.raw}).with_prefix(path);
            }

            std::vector<aast::VariableStatement *> arguments;

            for (auto *var : func->arguments) {
                arguments.push_back(new aast::VariableStatement(var->origin, var->type, var->name));
            }

            declarations.emplace(name,
                                 new aast::FuncDeclareStatement(statement->origin,
                                                                name,
                                                                func->return_type,
                                                                arguments,
                                                                func->var_arg));
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

    for (auto statement : statements) {
        std::optional verified = verify_statement(statement);
        if (verified.has_value())
            res.push_back(verified.value());
    }
    return res;
}

std::optional<aast::ScopeStatement *> Analyser::verify_scope(ast::ScopeStatement *scope, std::string name) {
    size_t old_var_count = variables.size();

    for (auto var : variables) {
        var->push_state(*var->state());
    }

    path = path.create_member(name);

    level++;

    std::optional statements = verify_statements(scope->block);

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
    last_loop = nullptr; // this shouldn't do anything, but just to be sure


    if (func->member_of.has_value())
        verify_type(func->member_of.value());

    Path func_path = Path({});
    if (func->member_of.has_value()) {
        func_path = func->member_of.value().get_path().with_prefix(path).create_member(func->name.raw);
    } else {
        func_path = Path({func->name.raw}).with_prefix(path);
    }

    for (auto registered : functions) {
        if (func_path != registered->path)
            continue;
        bucket->error(func->name.origin, "redefinition of '{}'", func->name.raw);
        bucket->note(registered->origin, "previous definition here");
    }

    bucket->iassert(func->return_type == Type(VOID) || does_always_return(func),
                    func->origin,
                    "function with return type doesn't always return");
    std::vector<aast::VariableStatement *> arguments;

    for (auto arg : func->arguments) {
        auto sem = verify_variable(arg);

        if (sem.has_value())
            sem.value()->state()->make_definitely_defined(arg->origin);

        arguments.push_back(new aast::VariableStatement(arg->origin, arg->type, arg->name));
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
    } else
        return {};
}

std::optional<aast::IfStatement *> Analyser::verify_if(ast::IfStatement *if_) {
    std::optional condition = verify_expression(if_->condition);
    std::optional scope = verify_scope(if_);
    if (if_->else_statement)
        verify_scope(if_->else_statement);

    if (condition.has_value() && scope.has_value()) {
        // implicitely compare to 0 if condition isn't a bool already
        if (condition.value()->type != Type(BOOL, 0)) {
            condition = new aast::BinaryOperatorExpression(if_->condition->origin,
                                                           Type(BOOL),
                                                           aast::NEQ,
                                                           condition.value(),
                                                           new aast::IntExpression(if_->condition->origin, 0));
        }

        std::vector block = std::move(scope.value()->block);
        delete scope.value();
        return new aast::IfStatement(if_->origin, condition.value(), block);
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
            bucket->iassert(return_type.is_compatible(value.value()->type),
                            return_->value->origin,
                            "can't return value of type '{}' in function with return type '{}'",
                            value.value()->type.str(),
                            return_type.str());

            return new aast::ReturnStatement(return_->origin, value.value());
        }
    } else {
        bucket->iassert(return_type == Type(VOID),
                        return_->origin,
                        "function with return type should return a value");

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
            condition = new aast::BinaryOperatorExpression(while_->condition->origin,
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
    bucket->iassert(last_loop, break_->origin, "break outside of loop");
    return new aast::BreakStatement(break_->origin);
}

std::optional<aast::ContinueStatement *> Analyser::verify_continue(ast::ContinueStatement *continue_) {
    bucket->iassert(last_loop, continue_->origin, "continue outside of loop");
    return new aast::ContinueStatement(continue_->origin);
}

std::optional<SemanticVariable *> Analyser::verify_variable(ast::VariableStatement *var) {
    std::optional type = verify_type(var->type);

    for (auto variable : variables) {
        if (var->name.raw == variable->var->name.raw) {
            bucket->error(var->name.origin, "redefinition of '{}'", var->name.raw);
            bucket->note(variable->var->name.origin, "previous definition here");
        }
    }

    if (!type.has_value())
        return {};

    auto *new_var = new aast::VariableStatement(var->origin, type.value(), var->name);

    SemanticVariable *sem;
    if (type.value().is_primitive()) {
        sem = new PrimitiveVariable(new_var);
    } else {
        aast::StructStatement *st = get_struct(Path(type.value().get_user()));

        std::vector<SemanticVariable *> member_states;
        for (auto member : st->members) {
            auto *temp = new ast::VariableStatement(var->origin,
                                                    member->type,
                                                    Token::name(var->name.raw + "." + member->name.raw));

            auto semantic_member = verify_variable(temp);
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
    Path struct_path = Path({struct_->name.raw}).with_prefix(path);

    for (auto [path, registered] : structures) {
        if (path != struct_path)
            continue;
        bucket->error(struct_->name.origin, "redefinition of '{}'", struct_->name.raw);
        bucket->note(registered->origin, "previous definition here");
    }

    std::vector<aast::VariableStatement *> members;
    std::vector<ast::VariableStatement *> ctor_args;
    std::vector<ast::Statement *> body;

    auto *instance = new ast::VariableStatement(struct_->origin, struct_->get_type(path), Token::name("_instance"));
    body.push_back(instance);

    for (auto member : struct_->members) {
        bucket->iassert(std::find(registered.begin(), registered.end(), member->name.raw) == registered.end(),
                        member->name.origin,
                        "duplicate member '{}'",
                        member->name.raw);

        std::optional member_type = verify_type(member->type);

        registered.push_back(member->name.raw);
        if (member_type.has_value()) {
            members.push_back(new aast::VariableStatement(member->origin, member_type.value(), member->name));
            ctor_args.push_back(new ast::VariableStatement(struct_->origin, member_type.value(), member->name));

            auto *member_access = new ast::BinaryOperatorExpression(struct_->origin,
                                                                    ast::MEM_ACC,
                                                                    new ast::NameExpression(
                                                                        struct_->origin,
                                                                        "_instance"),
                                                                    new ast::NameExpression(
                                                                        struct_->origin,
                                                                        member->name.raw));

            body.push_back(new ast::BinaryOperatorExpression(struct_->origin,
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

    path = path.create_member(struct_->name.raw);
    // fixme: this throws additional errors if member types are invalid
    std::optional constructor = verify_function(ctor);
    path = path.get_parent();

    bucket->iassert(constructor.has_value(),
                    struct_->origin,
                    "internal: failed to verify constructor for '{}'",
                    struct_path.str());
    declarations.emplace(constructor_path,
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
                                                              bool assigned_to,
                                                              bool member_acc) {
    switch (expression->expression_type) {
        case ast::CALL_EXPR: {
            auto ce = (ast::CallExpression *) expression;

            Path func_path = Path({});
            if (auto *gl = (ast::PrefixOperatorExpression *) ce->callee;
                ce->callee->expression_type == ast::NAME_EXPR || ce->callee->expression_type == ast::PATH_EXPR ||
                (ce->callee->expression_type == ast::PREFIX_EXPR && gl->prefix_type == ast::GLOBAL)) {
                func_path = Path::from_expression(ce->callee);

                if (is_struct_declared(func_path)) {
                    func_path = func_path.create_member("$constructor");
                    // TODO: check if there exists a function with the same name
                }
            } else if (ce->callee->expression_type == ast::MEM_ACC_EXPR) {
                auto *mem = (ast::BinaryOperatorExpression *) ce->callee;
                std::optional callee_parent = verify_expression(mem->left);

                if (mem->right->expression_type != ast::NAME_EXPR) {
                    bucket->error(mem->right->origin, "expected function name");
                    return {};
                }

                if (callee_parent.has_value()) {
                    func_path = Path(callee_parent.value()->type.get_path());
                    func_path = func_path.create_member(((ast::NameExpression *) mem->right)->name);
                } else {
                    return {};
                }

                ce->arguments.insert(ce->arguments.begin(), mem->left);
                mem->left = nullptr;
            } else {
                bucket->error(ce->callee->origin, "calling of expressions is unimplemented");
                return {};
            }

            // fixme: this needs to be a path expression kind of thing
            // fixme: in the very far future, this should have a function pointer type
            auto *callee = new aast::NameExpression(ce->callee->origin, Type(), func_path.str());

            if (!bucket->iassert(is_func_declared(func_path),
                                 ce->callee->origin,
                                 "undefined function '{}'",
                                 func_path.str()))
                return {};
            aast::FuncStCommon *func = get_func_decl(func_path);

            size_t i = 0;
            if (is_struct_declared(func->path.get_parent())) {
                if (func->arguments.size() > 0 && func->arguments[0]->name.raw == "this")
                    i = 1;
                else if (func->path.get_parts().back() != "$constructor")
                    // constructors are in the scope of a struct, but aren't called by a member expression. Otherwise
                    // function is static, but we always put in the this argument
                    ce->arguments.erase(ce->arguments.begin());
            }

            if (!bucket->iassert(ce->arguments.size() >= func->arguments.size(),
                                 ce->origin,
                                 "too few arguments, expected {} found {}.",
                                 func->arguments.size(),
                                 ce->arguments.size()))
                return {};

            if (!bucket->iassert(func->var_arg || ce->arguments.size() <= func->arguments.size(),
                                 ce->origin,
                                 "too many arguments, expected {} found {}.",
                                 func->arguments.size(),
                                 ce->arguments.size()))
                return {};

            std::vector<aast::Expression *> arguments;

            for (; i < std::min(func->arguments.size(), ce->arguments.size()); i++) {
                aast::VariableStatement *arg_var = func->arguments[i];
                ast::Expression *arg = ce->arguments[i];

                std::optional argument = verify_expression(arg);

                if (argument.has_value()) {
                    arguments.push_back(argument.value());

                    bucket->iassert(arg_var->type.is_compatible(argument.value()->type),
                                    arg->origin,
                                    "passing value of type '{}' to argument of type '{}'",
                                    argument.value()->type.str(),
                                    arg_var->type.str());
                }
            }


            return new aast::CallExpression(expression->origin, func->return_type, callee, arguments);
        }
        case ast::DASH_EXPR:
        case ast::DOT_EXPR:
        case ast::EQ_EXPR:
        case ast::COMP_EXPR:
        case ast::ASSIGN_EXPR: {
            auto ae = (ast::BinaryOperatorExpression *) expression;

            std::optional left = verify_expression(ae->left, expression->expression_type == ast::ASSIGN_EXPR);
            std::optional right = verify_expression(ae->right);

            if (!left.has_value() || !right.has_value())
                return {};

            bucket->iassert(left.value()->type.is_compatible(right.value()->type),
                            ae->right->origin,
                            "can't assign to type '{}' from '{}'",
                            left.value()->type.str(),
                            right.value()->type.str());

            Type expression_type;
            if (expression->expression_type == ast::EQ_EXPR || expression->expression_type == ast::COMP_EXPR)
                expression_type = Type(BOOL);
                // Todo: this is wrong; we generate LLVM code that casts to float if either operand is a float and
                //  to the highest bit width of the two operand
            else
                expression_type = left.value()->type;

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

            return new aast::BinaryOperatorExpression(ae->origin,
                                                      expression_type,
                                                      bot,
                                                      left.value(),
                                                      right.value());
        }
        case ast::MEM_ACC_EXPR: {
            auto mae = (ast::BinaryOperatorExpression *) expression;

            std::optional left = verify_expression(mae->left, assigned_to, true);

            if (!left.has_value())
                return {};

            if (!bucket->iassert(!left.value()->type.is_primitive(),
                                 left.value()->origin,
                                 "'{}' is not a structure",
                                 left.value()->type.str()))
                return {};
            auto struct_name = left.value()->type.get_user();
            if (!bucket->iassert(is_struct_declared(Path(struct_name)),
                                 left.value()->origin,
                                 "undefined structure '{}'",
                                 left.value()->type.str()))
                return {};
            aast::StructStatement *s = get_struct(Path(struct_name));

            if (!bucket->iassert(mae->right->expression_type == ast::NAME_EXPR,
                                 mae->right->origin,
                                 "expected identifier"))
                return {};

            std::string member_name = ((ast::NameExpression *) mae->right)->name;

            if (!bucket->iassert(s->has_member(member_name),
                                 mae->right->origin,
                                 "no member named '{}' in '{}'",
                                 member_name,
                                 left.value()->type.str()))
                return {};

            Type member_type = s->get_member_type(member_name);

            if (left.value()->flattens_to_member_access()) {
                auto *name = new ast::NameExpression(mae->origin,
                                                     left.value()->flatten_to_member_access() + "." + member_name);

                verify_expression(name, assigned_to);
                delete name;
            }

            return new aast::BinaryOperatorExpression(mae->origin,
                                                      member_type,
                                                      aast::MEM_ACC,
                                                      left.value(),
                                                      new
                                                      aast::NameExpression(mae->right->origin, Type(), member_name));
        }
        case ast::PREFIX_EXPR: {
            auto pe = (ast::PrefixOperatorExpression *) expression;
            std::optional operand = verify_expression(pe->operand);

            if (!operand.has_value())
                return {};

            Type pe_type = operand.value()->type;

            switch (pe->prefix_type) {
                case ast::NEG:
                case ast::LOG_NOT:
                    bucket->iassert(pe_type.is_primitive() || pe_type.pointer_level > 0,
                                    pe->operand->origin,
                                    "invalid operand to unary expression");
                    break;
                case ast::REF: {
                    pe_type.pointer_level++;
                    ast::ExprType etype = pe->operand->expression_type;

                    if (!bucket->iassert(etype == ast::NAME_EXPR || etype == ast::MEM_ACC_EXPR,
                                         pe->operand->origin,
                                         "cannot take reference of temporary value")) {
                        bucket->note(pe->operand->origin,
                                     "'{}' produces a temporary value",
                                     pe->operand->print());
                    }
                    break;
                }
                case ast::DEREF:
                    bucket->iassert(pe_type.pointer_level > 0,
                                    pe->operand->origin,
                                    "cannot dereference non-pointer type '{}'",
                                    operand.value()->type.str());
                    pe_type.pointer_level--;
                    break;
                case ast::GLOBAL:
                    bucket->error(pe->origin, "internal: unhandled global path prefix");
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

            return new aast::PrefixOperatorExpression(pe->origin, pe_type, pt, operand.value());
        }
        case ast::NAME_EXPR: {
            auto ne = (ast::NameExpression *) expression;
            if (!bucket->iassert(is_var_declared(ne->name),
                                 expression->origin,
                                 "undefined variable '{}'",
                                 ne->name))
                return {};

            SemanticVariable *var = get_variable(ne->name);

            Type variable_type = var->var->type;

            auto *new_expr = new aast::NameExpression(ne->origin, variable_type, ne->name);

            // don't do variable state checking if we are in a (pure) member access
            if (member_acc)
                return new_expr;

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
                if (bucket->iassert(!var->state()->is_definitely_undefined(),
                                    expression->origin,
                                    "value is still undefined")) {
                    bucket->iassert(!var->state()->is_maybe_undefined(),
                                    expression->origin,
                                    "value might be undefined");
                }

                var->state()->make_definitely_read(expression->origin);
            }

            return new_expr;
        }
        case ast::INT_EXPR:
            return new aast::IntExpression(expression->origin, ((ast::IntExpression *) expression)->n);
        case ast::REAL_EXPR:
            return new aast::RealExpression(expression->origin, ((ast::RealExpression *) expression)->n);
        case ast::STR_EXPR:
            return new aast::StringExpression(expression->origin, ((ast::StringExpression *) expression)->n);
        case ast::BOOL_EXPR:
            return new aast::BoolExpression(expression->origin, ((ast::BoolExpression *) expression)->n);
        default:
            // todo: do more analysis here:
            //  if it's a path, put an error after the expression: if the path points to a valid function, suggest
            //  calling it. if it points to a valid struct, suggest creating a variable with it
            bucket->error(expression->origin, "internal: unexpected, unhandled type of expression");
    }
    return {};
}

std::optional<Type> Analyser::verify_type(Type type) {
    if (!type.is_primitive()) {
        auto path = type.get_user();
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

        bucket->iassert(is_struct_declared(Path(path)),
                        type.origin,
                        "undefied type '{}'",
                        Path(path).str());

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

bool Analyser::is_var_declared(const std::string &name) {
    return std::find_if(variables.begin(),
                        variables.end(),
                        [name](SemanticVariable *v) {
                            return name == v->var->name.raw;
                        }) != variables.end();
}

bool Analyser::is_func_declared(Path path) {
    if (path.is_global()) {
        return declarations.contains(path);
    }

    return declarations.contains(path) || declarations.contains(path.with_prefix(Path(this->path)));
}

bool Analyser::is_struct_declared(Path path) {
    return structures.contains(path);
}

SemanticVariable *Analyser::get_variable(const std::string &name) {
    return *std::find_if(variables.begin(),
                         variables.end(),
                         [name](SemanticVariable *v) {
                             return name == v->var->name.raw;
                         });
}

aast::FuncStCommon *Analyser::get_func_decl(Path path) {
    if (path.is_global()) {
        return declarations[path];
    }

    Path local = path.with_prefix(Path(this->path));

    if (declarations.contains(local))
        return declarations[local];

    return declarations[path];
}

aast::StructStatement *Analyser::get_struct(Path path) {
    if (path.is_global()) {
        return structures[path];
    }

    Path local = path.with_prefix(Path(this->path));

    if (structures.contains(local))
        return structures[local];

    return structures[path];
}

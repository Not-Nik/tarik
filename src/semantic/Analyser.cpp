// tarik (c) Nikolas Wipper 2021

#include "Analyser.h"

#include "util/Util.h"
#include "syntactic/expressions/Expression.h"

bool Analyser::verify_statement(Statement *statement) {
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
    for (auto statement: statements) {
        // Combining these two lines will cause clang to not call `verify_statement` when res is false, effectively limiting us to a single error
        bool t = verify_statement(statement);
        res = res and t;
    }
    return res;
}

bool Analyser::verify_scope(ScopeStatement *scope) {
    size_t old_var_count = variables.size();
    size_t old_struct_count = structures.size();

    bool res = verify_statements(scope->block);

    while (old_var_count < variables.size()) variables.pop_back();
    while (old_struct_count < structures.size()) structures.pop_back();

    return res;
}

bool Analyser::verify_function(FuncStatement *func) {
    variables.clear();
    last_loop = nullptr; // this shouldn't do anything, but just to be sure

    for (auto registered: functions) {
        if (registered->name != func->name) continue;
        error(func->origin, "redefinition of '%s'", func->name.c_str());
        note(registered->origin, "previous definition here");
        return false;
    }

    for (auto decl: declarations) {
        if (decl->name != func->name) continue;
        if (!decl->definable) {
            error(func->origin, "redefinition of '%s'", func->name.c_str());
            note(decl->origin, "previous definition in imported file '%s'", decl->origin.filename.filename().c_str());
        } else if (decl->return_type != func->return_type) {
            error(func->origin, "definition of '%s' with different type from declaration", decl->name.c_str());
            note(decl->origin, "declaration here");
        }
        return false;
    }

    if (!iassert(func->return_type == Type(VOID) || does_always_return(func), func->origin, "function with return type doesn't always return"))
        return false;
    functions.push_back(func);
    for (auto arg: func->arguments) {
        variables.push_back(arg);
    }

    if (func->var_arg) warning(func->origin, "function uses var args, but they cannot be accessed");

    return verify_scope(func);
}

bool Analyser::verify_func_decl(FuncDeclareStatement *decl) {
    bool warned = false, func_warned = false, decl_warned = false;
    for (auto registered: functions) {
        if (registered->name != decl->name) continue;
        bool critical = registered->return_type != decl->return_type;
        if (!critical) {
            if (!func_warned) warning(decl->origin, "declaration of already defined function '%s'", decl->name.c_str());
        } else
            error(decl->origin, "redeclaration of '%s' with different type", decl->name.c_str());
        note(registered->origin, "%s definition here", critical ? "" : "previous ");
        if (critical) return false;
        warned = true;
        func_warned = true;
    }

    for (auto d: declarations) {
        if (d->name != decl->name) continue;
        bool critical = d->return_type != decl->return_type;
        if (!critical) {
            if (!decl_warned) warning(decl->origin, "declaration of already declared function '%s'", decl->name.c_str());
        } else
            error(decl->origin, "redeclaration of '%s' with different type", decl->name.c_str());
        note(d->origin, "previous declaration here");
        if (critical) return false;
        warned = true;
        decl_warned = true;
    }

    if (!warned) declarations.push_back(decl);
    return true;
}

bool Analyser::verify_if(IfStatement *if_) {
    return verify_expression(if_->condition) && verify_scope(if_) && (!if_->else_statement || verify_scope(if_->else_statement));
}

bool Analyser::verify_else(ElseStatement *else_) {
    error(else_->origin, "Else, but no preceding if");
    return false;
}

bool Analyser::verify_return(ReturnStatement *return_) {
    return verify_expression(return_->value);
}

bool Analyser::verify_while(WhileStatement *while_) {
    Statement *old_last_loop = last_loop;
    last_loop = while_;
    bool res = verify_expression(while_->condition) && verify_scope(while_);
    last_loop = old_last_loop;
    return res;
}

bool Analyser::verify_break(BreakStatement *break_) {
    return iassert(last_loop, break_->origin, "break outside of loop");
}

bool Analyser::verify_continue(ContinueStatement *continue_) {
    return iassert(last_loop, continue_->origin, "break outside of continue");
}

bool Analyser::verify_variable(VariableStatement *var) {
    for (auto variable: variables) {
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

    for (auto structure: structures) {
        if (struct_->name == structure->name) {
            error(struct_->origin, "redefinition of '%s'", struct_->name.c_str());
            note(structure->origin, "previous definition here");
            return false;
        }
    }

    for (auto member: struct_->members) {
        if (!iassert(std::find(registered.begin(), registered.end(), member->name) == registered.end(),
                     member->origin,
                     "duplicate member '%s'",
                     member->name.c_str()))
            return false;
        registered.push_back(member->name);
    }
    structures.push_back(struct_);
    return true;
}

bool Analyser::verify_import(ImportStatement *import) {
    return verify_statements(import->block);
}

bool Analyser::verify_expression(Expression *expression) {
    switch (expression->expression_type) {
        case CALL_EXPR: {
            auto ce = (CallExpression *) expression;
            if (ce->callee->expression_type == NAME_EXPR) {
                std::string func_name = ((NameExpression *) ce->callee)->name;
                if (!iassert(is_func_declared(func_name), ce->origin, "undefined function '%s'", func_name.c_str())) return false;
                FuncStCommon *func = get_func_decl(func_name);
                if (!iassert(ce->arguments.size() >= func->arguments.size(),
                             ce->callee->origin,
                             "too few arguments, expected %i found %i.",
                             func->arguments.size(),
                             ce->arguments.size()) ||

                    !iassert(func->var_arg || ce->arguments.size() <= func->arguments.size(),
                             ce->callee->origin,
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
            } else {
                return iassert(false, ce->origin, "calling of expressions is unimplemented");
            }
            break;
        }
        case DASH_EXPR:
        case DOT_EXPR:
        case EQ_EXPR:
        case COMP_EXPR:
        case ASSIGN_EXPR: {
            auto ae = (BinaryOperatorExpression *) expression;
            if (!verify_expression(ae->left) || !verify_expression(ae->right)
                || !iassert(ae->left->type.is_compatible(ae->right->type), ae->origin, "invalid operands to binary expression"))
                return false;
            if (expression->expression_type == EQ_EXPR || expression->expression_type == COMP_EXPR) ae->assign_type(Type(BOOL));
                // Todo: this is wrong; we generate LLVM code that casts to float if either operand is a float and
                //  to the highest bit width of the two operand
            else ae->assign_type(ae->left->type);
            break;
        }
        case MEM_ACC_EXPR: {
            auto mae = (BinaryOperatorExpression *) expression;
            auto left = mae->left;
            auto right = mae->right;

            if (!verify_expression(left)) return false;

            if (!iassert(!left->type.is_primitive, left->origin, "'%s' is not a structure", left->type.str().c_str())) return false;
            StructStatement *s = left->type.type.user_type;
            if (!iassert(right->expression_type == NAME_EXPR, right->origin, "expected identifier")) return false;
            std::string member_name = ((NameExpression *) right)->name;

            if (!iassert(s->has_member(member_name), right->origin, "no member named '%s' in '%s'", member_name.c_str(), left->type.str().c_str()))
                return false;
            mae->assign_type(s->get_member_type(member_name));
            break;
        }
        case PREFIX_EXPR: {
            auto pe = (PrefixOperatorExpression *) expression;
            if (!verify_expression(pe->operand)) return false;

            bool res = true;
            Type pe_type = pe->operand->type;

            switch (pe->prefix_type) {
                case POS:
                case NEG:
                case LOG_NOT:
                    res = iassert(pe_type.is_primitive || pe_type.pointer_level > 0, pe->origin, "invalid operand to unary expression");
                    break;
                case REF: {
                    pe_type.pointer_level++;
                    ExprType etype = pe->operand->expression_type;
                    res = iassert(etype == NAME_EXPR || etype == MEM_ACC_EXPR, pe->origin, "cannot take reference of temporary value");
                    if (!res) note(pe->operand->origin, "'%s' produces a temporary value", pe->operand->print().c_str());
                    break;
                }
                case DEREF:
                    res = iassert(pe_type.pointer_level > 0, pe->origin, "cannot dereference non-pointer type '%s'", pe->operand->type.str().c_str());
                    pe_type.pointer_level--;
            }

            pe->assign_type(pe_type);
            return res;
        }
        case NAME_EXPR: {
            auto ne = (NameExpression *) expression;
            if (!iassert(is_var_declared(ne->name), expression->origin, "undefined variable '%s'", ne->name.c_str())) return false;
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
    for (auto &it: scope->block) {
        if ((it->statement_type == RETURN_STMT) || (it->statement_type == SCOPE_STMT && does_always_return((ScopeStatement *) it))
            || (it->statement_type == IF_STMT && ((IfStatement *) it)->else_statement && does_always_return((ScopeStatement *) it)
                && does_always_return(((IfStatement *) it)->else_statement))
            || (it->statement_type == WHILE_STMT && does_always_return((ScopeStatement *) it)))
            return true;
    }
    return false;
}

bool Analyser::is_var_declared(const std::string &name) {
    return std::find_if(variables.begin(), variables.end(), [name](VariableStatement *v) { return name == v->name; }) != variables.end();
}

bool Analyser::is_func_declared(const std::string &name) {
    return std::find_if(functions.begin(), functions.end(), [name](FuncStatement *v) { return name == v->name; }) != functions.end()
        || std::find_if(declarations.begin(), declarations.end(), [name](FuncDeclareStatement *v) { return name == v->name; }) != declarations.end();
}

FuncStCommon *Analyser::get_func_decl(const std::string &name) {
    auto fun = std::find_if(functions.begin(), functions.end(), [name](FuncStatement *v) { return name == v->name; });
    if (fun != functions.end()) return *fun;

    auto decl = std::find_if(declarations.begin(), declarations.end(), [name](FuncDeclareStatement *v) { return name == v->name; });
    if (decl != declarations.end()) return *decl;
    return nullptr;
}

VariableStatement *Analyser::get_variable(const std::string &name) {
    return *std::find_if(variables.begin(), variables.end(), [name](VariableStatement *v) { return name == v->name; });
}

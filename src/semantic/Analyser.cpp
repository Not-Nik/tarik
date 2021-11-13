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
        case EXPR_STMT:
            return verify_expression((Expression *) statement);
    }
    return true;
}

bool Analyser::verify_statements(std::vector<Statement *> statements) {
    for (auto statement: statements) {
        if (!verify_statement(statement)) return false;
    }
    return true;
}

bool Analyser::verify_scope(ScopeStatement *scope) {
    return verify_statements(scope->block);
}

bool Analyser::verify_function(FuncStatement *func) {
    for (auto registered: functions) {
        if (registered->name != func->name) continue;
        error(func->origin, "redefinition of '%s'", func->name.c_str());
        note(registered->origin, "previous definition here");
        return false;
    }

    iassert(func->return_type.operator==(Type(VOID)) || does_always_return(func), func->origin, "function with return type doesn't return");
    functions.push_back(func);
    return verify_scope(func);
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
    return verify_expression(while_->condition) && verify_scope(while_);
}

bool Analyser::verify_break(BreakStatement *break_) {
    return true;
}

bool Analyser::verify_continue(ContinueStatement *continue_) {
    return true;
}

bool Analyser::verify_variable(VariableStatement *var) {
    bool res = iassert(!is_var_declared(var->name), var->origin, "Redefinition of '%s'", var->name.c_str());
    if (res) variables.push_back(var);
    return res;
}

bool Analyser::verify_struct(StructStatement *struct_) {
    return true;
}

bool Analyser::verify_expression(Expression *expression) {
    switch (expression->expression_type) {
        case CALL_EXPR: {
            auto ce = (CallExpression *) expression;
            if (ce->callee->expression_type == NAME_EXPR) {
                std::string func_name = ((NameExpression *) ce->callee)->name;
                iassert(is_func_declared(func_name), ce->origin, "Undefined function '%s'", func_name.c_str());
                FuncStatement
                    *func = *std::find_if(functions.begin(), functions.end(), [func_name](FuncStatement *v) { return func_name == v->name; });
                if (!iassert(ce->arguments.size() >= func->arguments.size(),
                             ce->callee->origin,
                             "Too few arguments, expected %i found %i.",
                             func->arguments.size(),
                             ce->arguments.size()) ||

                    !iassert(ce->arguments.size() <= func->arguments.size(),
                             ce->callee->origin,
                             "Too many arguments, expected %i found %i.",
                             func->arguments.size(),
                             ce->arguments.size()))
                    return false;

                for (size_t i = 0; i < func->arguments.size(); i++) {
                    VariableStatement *arg_var = func->arguments[i];
                    Expression *arg = ce->arguments[i];
                    if (!iassert(arg_var->type.is_compatible(arg->get_type()),
                                 arg->origin,
                                 "Passing value of type '%s' to argument of type '%s'",
                                 arg->get_type().str().c_str(),
                                 arg_var->type.str().c_str()) || !verify_expression(arg))
                        return false;
                }
            } else {
                throw "calling of expressions is unimplemented";
            }
            break;
        }
        case DASH_EXPR:
        case DOT_EXPR:
        case EQ_EXPR:
        case COMP_EXPR:
        case ASSIGN_EXPR: {
            auto ae = (BinaryOperatorExpression *) expression;
            return verify_expression(ae->left) && verify_expression(ae->right)
                && iassert(ae->left->get_type().is_compatible(ae->right->get_type()), ae->origin, "Invalid operands to binary expression");
        }
        case PREFIX_EXPR: {
            auto pe = (PrefixOperatorExpression *) expression;
            return verify_expression(pe->operand)
                && iassert(pe->get_type().is_primitive || pe->get_type().pointer_level > 0, pe->origin, "Invalid operand to unary expression");
        }
        case NAME_EXPR: {
            auto ne = (NameExpression *) expression;
            return iassert(is_var_declared(ne->name), expression->origin, "Undefined variable '%s'", ne->name.c_str());
        }
        case INT_EXPR:
        case REAL_EXPR:
            break;
    }
    return true;
}

bool Analyser::does_always_return(ScopeStatement *scope) {
    for (auto it = scope->block.begin(); it != scope->block.end(); it++) {
        if (((*it)->statement_type == RETURN_STMT) || ((*it)->statement_type == SCOPE_STMT && does_always_return((ScopeStatement *) (*it)))
            || ((*it)->statement_type == IF_STMT && does_always_return((ScopeStatement *) (*it))
                && (it + 1 == scope->block.end() || ((*(it + 1))->statement_type == ELSE_STMT && does_always_return((ScopeStatement *) (*++it)))))
            || ((*it)->statement_type == WHILE_STMT && does_always_return((ScopeStatement *) (*it))))
            return true;
    }
    return false;
}

bool Analyser::is_var_declared(const std::string &name) {
    return std::find_if(variables.begin(), variables.end(), [name](VariableStatement *v) { return name == v->name; }) != variables.end();
}

bool Analyser::is_func_declared(const std::string &name) {
    return std::find_if(functions.begin(), functions.end(), [name](FuncStatement *v) { return name == v->name; }) != functions.end();
}

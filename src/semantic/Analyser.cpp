// tarik (c) Nikolas Wipper 2021

#include "Analyser.h"

#include "util/Util.h"

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
    return true;
}

bool Analyser::verify_else(ElseStatement *else_) {
    return true;
}

bool Analyser::verify_return(ReturnStatement *return_) {
    return true;
}

bool Analyser::verify_while(WhileStatement *while_) {
    return true;
}

bool Analyser::verify_break(BreakStatement *break_) {
    return true;
}

bool Analyser::verify_continue(ContinueStatement *continue_) {
    return true;
}

bool Analyser::verify_variable(VariableStatement *var) {
    return true;
}

bool Analyser::verify_struct(StructStatement *struct_) {
    return true;
}

bool Analyser::verify_expression(Expression *expression) {
    return true;
}

bool Analyser::does_always_return(ScopeStatement *scope) {
    for (auto it = scope->block.begin(); it != scope->block.end(); it++) {
        if ((*it)->statement_type == RETURN_STMT) return true;
        else if ((*it)->statement_type == SCOPE_STMT && does_always_return((ScopeStatement *) (*it))) return true;
        else if ((*it)->statement_type == IF_STMT && does_always_return((ScopeStatement *) (*it))
            && (it + 1 == scope->block.end() || ((*(it + 1))->statement_type == ELSE_STMT && does_always_return((ScopeStatement *) (*++it)))))
            return true;
        else if ((*it)->statement_type == WHILE_STMT && does_always_return((ScopeStatement *) (*it))) return true;
    }
    return false;
}

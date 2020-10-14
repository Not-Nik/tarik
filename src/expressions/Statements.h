// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_STATEMENTS_H
#define TARIK_STATEMENTS_H

#include <map>
#include <utility>

#include "Types.h"

class Expression;

enum StmtType {
    FUNC_STMT,
    IF_STMT,
    ELSE_STMT,
    RETURN_STMT,
    WHILE_STMT,
    FOR_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    VARIABLE_STMT,
    STRUCT_STMT,
    EXPR_STMT
};

class Statement {
public:
    StmtType statement_type;

    explicit Statement(StmtType t) {
        statement_type = t;
    }
};

class ScopeStatement : public Statement {
public:
    std::vector<Statement *> block;

    ScopeStatement(StmtType t, std::vector<Statement *> b) :
            Statement(t),
            block(std::move(b)) { }
};

class FuncStatement : public ScopeStatement {
public:
    std::string name;
    Type return_type;
    std::map<std::string, Type> arguments;

    FuncStatement(std::string n, Type ret, std::map<std::string, Type> args, std::vector<Statement *> b) :
            ScopeStatement(FUNC_STMT, std::move(b)),
            name(std::move(n)),
            return_type(ret),
            arguments(std::move(args)) { }
};

class IfStatement : public ScopeStatement {
public:
    Expression * condition;

    IfStatement(Expression * cond, std::vector<Statement *> b) :
            ScopeStatement(IF_STMT, std::move(b)),
            condition(cond) {
    }
};

class ElseStatement : public ScopeStatement {
public:
    IfStatement * inverse;

    explicit ElseStatement(IfStatement * inv, std::vector<Statement *> b) :
            ScopeStatement(ELSE_STMT, std::move(b)),
            inverse(inv) {
    }
};

class ReturnStatement : public Statement {
public:
    Expression * value;

    explicit ReturnStatement(Expression * val) :
            Statement(RETURN_STMT),
            value(val) {
    }
};

class WhileStatement : public ScopeStatement {
public:
    Expression * condition;

    explicit WhileStatement(Expression * cond, std::vector<Statement *> b) :
            ScopeStatement(WHILE_STMT, std::move(b)),
            condition(cond) {
    }
};

class ForStatement : public ScopeStatement {
public:
    Expression * initializer, * condition, * loop;

    ForStatement(Expression * init, Expression * cond, Expression * l, std::vector<Statement *> b) :
            ScopeStatement(FOR_STMT, std::move(b)),
            initializer(init),
            condition(cond),
            loop(l) {
    }
};

class BreakStatement : public Statement {
public:
    BreakStatement() : Statement(BREAK_STMT) { }
};

class ContinueStatement : public Statement {
public:
    ContinueStatement() : Statement(CONTINUE_STMT) { }
};

class VariableStatement : public Statement {
public:
    Type type;
    std::string name;

    VariableStatement(Type t, std::string n) :
            Statement(VARIABLE_STMT),
            type(t),
            name(std::move(n)) { }
};

#endif //TARIK_STATEMENTS_H

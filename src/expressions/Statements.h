// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_STATEMENTS_H
#define TARIK_STATEMENTS_H

#include <map>
#include <utility>

#include "Types.h"

class Expression;

enum StmtType {
    SCOPE_STMT,
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

    Statement() = default;

    explicit Statement(StmtType t) {
        statement_type = t;
    }

    [[nodiscard]] virtual std::string print() const { return ""; }
};

class FuncStatement : public Statement {
public:
    std::string name;
    Type return_type;
    std::vector<Statement *> block;
    std::map<std::string, Type> arguments;

    FuncStatement(std::string n, Type ret, std::map<std::string, Type> args, std::vector<Statement *> b) :
            Statement(FUNC_STMT),
            name(std::move(n)),
            return_type(ret),
            block(std::move(b)),
            arguments(std::move(args)) { }
};

class IfStatement : public Statement {
public:
    Expression * condition;
    Statement * then;

    IfStatement(Expression * cond, Statement * t) :
            Statement(IF_STMT),
            condition(cond),
            then(t) {
    }
};

class ElseStatement : public Statement {
public:
    IfStatement * inverse;
    Statement * then;

    explicit ElseStatement(IfStatement * inv, Statement * t) :
            Statement(ELSE_STMT),
            inverse(inv),
            then(t) {
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

class WhileStatement : public Statement {
public:
    Expression * condition;
    Statement * then;

    explicit WhileStatement(Expression * cond, Statement * t) :
            Statement(WHILE_STMT),
            condition(cond),
            then(t) {
    }
};

class ForStatement : public Statement {
public:
    Expression * initializer, * condition, * loop;
    Statement * then;

    ForStatement(Expression * init, Expression * cond, Expression * l, Statement * t) :
            Statement(FOR_STMT),
            initializer(init),
            condition(cond),
            loop(l),
            then(t) {
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

class StructStatement : public Statement {
public:
    std::string name;
    std::vector<VariableStatement *> members;
};

#endif //TARIK_STATEMENTS_H

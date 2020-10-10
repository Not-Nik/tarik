// Seno (c) Nikolas Wipper 2020

#ifndef SENO_STATEMENTS_H
#define SENO_STATEMENTS_H

#include "Expression.h"

enum StmtType {
    IF_STMT,
    ELSE_STMT,
    RETURN_STMT,
    WHILE_STMT,
    FOR_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    STRUCT_STMT
};

class Statement {
public:
    StmtType type;

    explicit Statement(StmtType t) {
        type = t;
    }
};

class IfStatement : public Statement {
public:
    Expression condition;

    explicit IfStatement(Expression cond) :
            Statement(IF_STMT),
            condition(cond) {
    }
};

class ElseStatement : public Statement {
public:
    IfStatement * inverse;

    explicit ElseStatement(IfStatement * inv) :
            Statement(ELSE_STMT),
            inverse(inv) {
    }
};

class ReturnStatement : public Statement {
public:
    Expression value;

    explicit ReturnStatement(Expression val) :
            Statement(RETURN_STMT),
            value(val) {
    }
};

class WhileStatement : public Statement {
public:
    Expression condition;

    explicit WhileStatement(Expression cond) :
            Statement(WHILE_STMT),
            condition(cond) {
    }
};

class ForStatement : public Statement {
public:
    Expression initializer, condition, loop;

    explicit ForStatement(Expression init, Expression cond, Expression l) :
            Statement(FOR_STMT),
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

#endif //SENO_STATEMENTS_H

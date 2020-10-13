#include <utility>

// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_STATEMENTS_H
#define TARIK_STATEMENTS_H


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
    STRUCT_STMT,
    EXPR_STMT
};

class Statement {
public:
    StmtType type;

    explicit Statement(StmtType t) {
        type = t;
    }
};

class FuncStatement : public Statement {

};

class IfStatement : public Statement {
public:
    Expression * condition;
    std::vector<Statement *> block;

    explicit IfStatement(Expression * cond, std::vector<Statement *> b) :
            Statement(IF_STMT),
            condition(cond),
            block(std::move(b)) {
    }
};

class ElseStatement : public Statement {
public:
    IfStatement * inverse;
    std::vector<Statement *> block;

    explicit ElseStatement(IfStatement * inv, std::vector<Statement *> b) :
            Statement(ELSE_STMT),
            inverse(inv),
            block(std::move(b)) {
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
    std::vector<Statement *> block;

    explicit WhileStatement(Expression * cond, std::vector<Statement *> b) :
            Statement(WHILE_STMT),
            condition(cond),
            block(std::move(b)) {
    }
};

class ForStatement : public Statement {
public:
    Expression * initializer, * condition, * loop;
    std::vector<Statement *> block;

    explicit ForStatement(Expression * init, Expression * cond, Expression * l, std::vector<Statement *> b) :
            Statement(FOR_STMT),
            initializer(init),
            condition(cond),
            loop(l),
            block(std::move(b)) {
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

#endif //TARIK_STATEMENTS_H

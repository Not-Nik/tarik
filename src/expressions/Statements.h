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
    StmtType statement_type {};

    Statement() = default;

    explicit Statement(StmtType t) {
        statement_type = t;
    }

    [[nodiscard]] virtual std::string print() const { return ""; }
};

class ScopeStatement : public Statement {
public:
    std::vector<Statement *> block;

    ScopeStatement(StmtType t, std::vector<Statement *> b) :
            Statement(t),
            block(std::move(b)) { }

    [[nodiscard]] std::string print() const override {
        std::string res;
        for (auto * st : block) {
            std::string t = st->print();
            if (st->statement_type == EXPR_STMT)
                t.push_back(';');
            res += t + "\n";
        }
        res.pop_back();
        return res;
    }
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

    [[nodiscard]] std::string print() const override {
        std::string res = "fn " + name + "(";
        for (auto arg : arguments) {
            res += "<todo: type to string> " + name + ", ";
        }
        if (res.back() != '(')
            res = res.substr(0, res.size() - 2);
        return res + ") <todo: type to string> {\n" + ScopeStatement::print() + "\n}";
    }
};

class IfStatement : public Statement {
public:
    // Statement so we can print it
    Statement * condition;
    Statement * then;

    IfStatement(Expression * cond, Statement * t) :
            Statement(IF_STMT),
            condition(reinterpret_cast<Statement *>(cond)),
            then(t) {
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = then->print();
        if (then->statement_type == EXPR_STMT)
            then_string += ";";
        return "if " + condition->print() + " {\n" + then_string + "\n}";
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

    [[nodiscard]] std::string print() const override {
        std::string then_string = then->print();
        if (then->statement_type == EXPR_STMT)
            then_string += ";";
        return "else " + then_string;
    }
};

class ReturnStatement : public Statement {
public:
    // Statement so we can print it
    Statement * value;

    explicit ReturnStatement(Expression * val) :
            Statement(RETURN_STMT),
            value(reinterpret_cast<Statement *>(val)) {
    }

    [[nodiscard]] std::string print() const override {
        return "return " + value->print() + ";";
    }
};

class WhileStatement : public Statement {
public:
    Statement * condition;
    Statement * then;

    explicit WhileStatement(Expression * cond, Statement * t) :
            Statement(WHILE_STMT),
            condition(reinterpret_cast<Statement *>(cond)),
            then(t) {
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = then->print();
        if (then->statement_type == EXPR_STMT)
            then_string += ";";
        return "while " + condition->print() + " {\n" + then_string + "\n}";
    }
};

class ForStatement : public Statement {
public:
    Statement * initializer, * condition, * loop;
    Statement * then;

    ForStatement(Expression * init, Expression * cond, Expression * l, Statement * t) :
            Statement(FOR_STMT),
            initializer(reinterpret_cast<Statement *>(init)),
            condition(reinterpret_cast<Statement *>(cond)),
            loop(reinterpret_cast<Statement *>(l)),
            then(t) {
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = then->print();
        if (then->statement_type == EXPR_STMT)
            then_string += ";";
        return "for " + initializer->print() + "; " + condition->print() + "; " + loop->print() + " {\n" + then_string + "\n}";
    }
};

class BreakStatement : public Statement {
public:
    BreakStatement() : Statement(BREAK_STMT) { }

    [[nodiscard]] std::string print() const override {
        return "break;";
    }
};

class ContinueStatement : public Statement {
public:
    ContinueStatement() : Statement(CONTINUE_STMT) { }

    [[nodiscard]] std::string print() const override {
        return "continue;";
    }
};

class VariableStatement : public Statement {
public:
    Type type;
    std::string name;

    VariableStatement(Type t, std::string n) :
            Statement(VARIABLE_STMT),
            type(t),
            name(std::move(n)) { }

    [[nodiscard]] std::string print() const override {
        return "<todo: type to string> " + name + ";";
    }
};

class StructStatement : public Statement {
public:
    std::string name;
    std::vector<VariableStatement *> members;

    [[nodiscard]] std::string print() const override {
        std::string res = "struct " + name + " {\n";
        for (auto * mem : members) {
            res += mem->print() + "\n";
        }
        return res + "\n}";
    }
};

#endif //TARIK_STATEMENTS_H

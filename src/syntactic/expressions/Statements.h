// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_

#include <map>
#include <utility>
#include <vector>
#include <utility>
#include <algorithm>

#include "Types.h"
#include "Base.h"

class ScopeStatement : public Statement {
public:
    std::vector<Statement *> block;

    ScopeStatement(StmtType t, const LexerPos &o, std::vector<Statement *> b)
        : Statement(t, o), block(std::move(b)) {}

    ~ScopeStatement() override {
        for (auto *st: block) {
            delete st;
        }
    }

    [[nodiscard]] std::string print() const override {
        std::string res = "{\n";
        for (auto *st: block) {
            std::string t = st->print();
            if (st->statement_type == EXPR_STMT)
                t.push_back(';');
            res += t + "\n";
        }
        res.pop_back();
        res += "\n}";
        return res;
    }
};

class IfStatement : public ScopeStatement {
public:
    Expression *condition;

    IfStatement(const LexerPos &o, Expression *cond, std::vector<Statement *> block)
        : ScopeStatement(IF_STMT, o, block), condition(cond) {
    }

    ~IfStatement() override {
        delete condition;
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = ScopeStatement::print();
        return "if " + condition->print() + " " + then_string;
    }
};

class ElseStatement : public ScopeStatement {
public:
    IfStatement *inverse;

    explicit ElseStatement(const LexerPos &o, IfStatement *inv, std::vector<Statement *> block)
        : ScopeStatement(ELSE_STMT, o, block), inverse(inv) {
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = ScopeStatement::print();
        return "else " + then_string;
    }
};

class ReturnStatement : public Statement {
public:
    // Statement, instead of expression, so we can print it
    Expression *value;

    explicit ReturnStatement(const LexerPos &o, Expression *val)
        : Statement(RETURN_STMT, o), value(val) {
    }

    ~ReturnStatement() override {
        delete value;
    }

    [[nodiscard]] std::string print() const override {
        return "return " + value->print() + ";";
    }
};

class WhileStatement : public ScopeStatement {
public:
    // Statement, instead of expression, so we can print it
    Statement *condition;

    explicit WhileStatement(const LexerPos &o, Expression *cond, std::vector<Statement *> block)
        : ScopeStatement(WHILE_STMT, o, block), condition(reinterpret_cast<Statement *>(cond)) {
    }

    ~WhileStatement() override {
        delete condition;
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = ScopeStatement::print();
        return "while " + condition->print() + " " + then_string;
    }
};

class BreakStatement : public Statement {
public:
    explicit BreakStatement(const LexerPos &o)
        : Statement(BREAK_STMT, o) {}

    [[nodiscard]] std::string print() const override {
        return "break;";
    }
};

class ContinueStatement : public Statement {
public:
    explicit ContinueStatement(const LexerPos &o)
        : Statement(CONTINUE_STMT, o) {}

    [[nodiscard]] std::string print() const override {
        return "continue;";
    }
};

class VariableStatement : public Statement {
public:
    Type type;
    std::string name;

    VariableStatement(const LexerPos &o, Type t, std::string n)
        : Statement(VARIABLE_STMT, o), type(t), name(std::move(n)) {}

    [[nodiscard]] std::string print() const override {
        return (std::string) type + " " + name + ";";
    }
};

class FuncStatement : public ScopeStatement {
public:
    std::string name;
    Type return_type;
    std::vector<VariableStatement *> arguments;

    FuncStatement(const LexerPos &o, std::string n, Type ret, std::vector<VariableStatement *> args, std::vector<Statement *> b)
        : ScopeStatement(FUNC_STMT, o, std::move(b)), name(std::move(n)), return_type(ret), arguments(std::move(args)) {}

    ~FuncStatement() override {
        for (auto *arg: arguments) {
            delete arg;
        }
    }

    [[nodiscard]] std::string head() const {
        std::string res = "fn " + name + "(";
        for (auto arg: arguments) {
            res += (std::string) arg->type + " " + arg->name + ", ";
        }
        if (res.back() != '(')
            res = res.substr(0, res.size() - 2);
        return res + ") " + (std::string) return_type;
    }

    [[nodiscard]] std::string print() const override {
        return head() + " " + ScopeStatement::print();
    }

    [[nodiscard]] std::string signature() const {
        std::string res = "(";
        for (auto arg: arguments) {
            res += (std::string) arg->type + ", ";
        }
        if (res.back() != '(') res = res.substr(0, res.size() - 2);
        return res + ") " + (std::string) return_type;
    }
};

class StructStatement : public Statement {
public:
    std::string name;
    std::vector<VariableStatement *> members;

    ~StructStatement() override {
        for (auto *m: members) {
            delete m;
        }
    }

    bool has_member(const std::string &n) {
        return std::any_of(members.front(), members.back(), [n](const VariableStatement &mem) {
            return mem.name == n;
        });
    }

    Type get_member_type(const std::string &n) {
        for (auto *mem: members) {
            if (mem->name == n)
                return mem->type;
        }
        return {};
    }

    [[nodiscard]] std::string print() const override {
        std::string res = "struct " + name + " {\n";
        for (auto *mem: members) {
            res += mem->print() + "\n";
        }
        return res + "\n}";
    }
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_

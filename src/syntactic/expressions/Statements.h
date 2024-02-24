// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_

#include <map>
#include <vector>

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

class ElseStatement : public ScopeStatement {
public:
    explicit ElseStatement(const LexerPos &o, std::vector<Statement *> block)
        : ScopeStatement(ELSE_STMT, o, std::move(block)) {
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = ScopeStatement::print();
        return "else " + then_string;
    }
};

class IfStatement : public ScopeStatement {
public:
    Expression *condition;
    ElseStatement *else_statement = nullptr;

    IfStatement(const LexerPos &o, Expression *cond, std::vector<Statement *> block)
        : ScopeStatement(IF_STMT, o, std::move(block)), condition(cond) {
    }

    ~IfStatement() override {
        delete condition;
        delete else_statement;
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = ScopeStatement::print();
        return "if " + condition->print() + " " + then_string;
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
    Expression *condition;

    explicit WhileStatement(const LexerPos &o, Expression *cond, std::vector<Statement *> block)
        : ScopeStatement(WHILE_STMT, o, block), condition(cond) {
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
        return type.str() + " " + name + ";";
    }
};

class FuncStCommon {
public:
    std::string name;
    Type return_type;
    std::vector<VariableStatement *> arguments;
    bool var_arg;

    FuncStCommon(std::string n, Type ret, std::vector<VariableStatement *> args, bool va)
        : name(std::move(n)), return_type(ret), arguments(std::move(args)), var_arg(va) {}

    [[nodiscard]] std::string head() const {
        std::string res = "fn " + name + "(";
        for (auto arg: arguments) {
            res += arg->type.str() + " " + arg->name + ", ";
        }
        if (res.back() != '(')
            res = res.substr(0, res.size() - 2);
        return res + ") " + return_type.str();
    }

    [[nodiscard]] std::string signature() const {
        std::string res = "(";
        for (auto arg: arguments) {
            res += arg->type.str() + ", ";
        }
        if (res.back() != '(') res = res.substr(0, res.size() - 2);
        return res + ") " + return_type.str();
    }
};

class FuncDeclareStatement : public Statement, public FuncStCommon {
public:
    FuncDeclareStatement(const LexerPos &o,
                         std::string n,
                         Type ret,
                         std::vector<VariableStatement *> args,
                         bool var_arg)
        : Statement(FUNC_DECL_STMT, o), FuncStCommon(std::move(n), ret, args, var_arg) {}

    [[nodiscard]] std::string print() const override {
        return head();
    }
};

class FuncStatement : public ScopeStatement, public FuncStCommon {
public:
    FuncStatement(const LexerPos &o,
                  std::string n,
                  Type ret,
                  std::vector<VariableStatement *> args,
                  std::vector<Statement *> b,
                  bool var_arg)
        : ScopeStatement(FUNC_STMT, o, std::move(b)), FuncStCommon(std::move(n), ret, std::move(args), var_arg) {}

    ~FuncStatement() override {
        for (auto *arg: arguments) {
            delete arg;
        }
    }

    [[nodiscard]] std::string print() const override {
        return head() + " " + ScopeStatement::print();
    }
};

class StructStatement : public Statement {
public:
    Token name;
    std::vector<VariableStatement *> members;

    StructStatement(const LexerPos &o, Token n, std::vector<VariableStatement *> m)
        : Statement(STRUCT_STMT, o), name(std::move(n)), members(std::move(m)) {}

    ~StructStatement() override {
        for (auto *m: members) {
            delete m;
        }
    }

    bool has_member(const std::string &n) {
        for (auto mem: members) {
            if (mem->name == n)
                return true;
        }
        return false;
    }

    Type get_type(std::vector<std::string> path = {}) {
        path.push_back(this->name.raw);
        return Type(path, 0);
    }

    Type get_member_type(const std::string &n) {
        for (auto mem: members) {
            if (mem->name == n)
                return mem->type;
        }
        return {};
    }

    unsigned int get_member_index(const std::string &n) {
        for (size_t i = 0; i < members.size(); i++) {
            if (members[i]->name == n) return i;
        }
        return -1;
    }

    [[nodiscard]] std::string print() const override {
        std::string res = "struct " + name.raw + " {\n";
        for (auto *mem: members) {
            res += mem->print() + "\n";
        }
        return res + "\n}";
    }
};

class ImportStatement : public ScopeStatement {
public:
    std::string name;

    ImportStatement(const LexerPos &o, std::string n, std::vector<Statement *> s)
        : ScopeStatement(IMPORT_STMT, o, std::move(s)), name(std::move(n)) {}
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_

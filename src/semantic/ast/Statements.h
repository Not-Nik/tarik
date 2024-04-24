// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_EXPRESSIONS_STATEMENTS_H_
#define TARIK_SRC_SEMANTIC_EXPRESSIONS_STATEMENTS_H_

#include <map>
#include <vector>

#include "Base.h"
#include "semantic/Path.h"
#include "syntactic/Types.h"

namespace aast
{
class ScopeStatement : public Statement {
public:
    std::vector<Statement *> block;

    ScopeStatement(StmtType t, const LexerRange &o, std::vector<Statement *> b)
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
    explicit ElseStatement(const LexerRange &o, std::vector<Statement *> block)
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

    IfStatement(const LexerRange &o, Expression *cond, std::vector<Statement *> block)
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

    explicit ReturnStatement(const LexerRange &o, Expression *val)
        : Statement(RETURN_STMT, o), value(val) {
    }

    ~ReturnStatement() override {
        delete value;
    }

    [[nodiscard]] std::string print() const override {
        if (value)
            return "return " + value->print() + ";";
        else
            return "return;";
    }
};

class WhileStatement : public ScopeStatement {
public:
    // Statement, instead of expression, so we can print it
    Expression *condition;

    explicit WhileStatement(const LexerRange &o, Expression *cond, std::vector<Statement *> block)
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
    explicit BreakStatement(const LexerRange &o)
        : Statement(BREAK_STMT, o) {}

    [[nodiscard]] std::string print() const override {
        return "break;";
    }
};

class ContinueStatement : public Statement {
public:
    explicit ContinueStatement(const LexerRange &o)
        : Statement(CONTINUE_STMT, o) {}

    [[nodiscard]] std::string print() const override {
        return "continue;";
    }
};

class VariableStatement : public Statement {
public:
    Type type;
    Token name;

    VariableStatement(const LexerRange &o, Type t, Token n)
        : Statement(VARIABLE_STMT, o), type(t), name(std::move(n)) {}

    [[nodiscard]] std::string print() const override {
        return type.str() + " " + name.raw + ";";
    }
};

class FuncStCommon {
public:
    Path path;
    Type return_type;
    std::vector<VariableStatement *> arguments;
    bool var_arg;

    FuncStCommon(Path p, Type ret, std::vector<VariableStatement *> args, bool va)
        : path(std::move(p)), return_type(ret), arguments(std::move(args)), var_arg(va) {}

    [[nodiscard]] std::string head() const {
        std::string res = "fn " + path.str() + "(";
        for (auto arg: arguments) {
            res += arg->type.str() + " " + arg->name.raw + ", ";
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
    FuncDeclareStatement(const LexerRange &o,
                         Path p,
                         Type ret,
                         std::vector<VariableStatement *> args,
                         bool var_arg)
        : Statement(FUNC_DECL_STMT, o), FuncStCommon(std::move(p), ret, args, var_arg) {}

    [[nodiscard]] std::string print() const override {
        return head();
    }
};

class FuncStatement : public ScopeStatement, public FuncStCommon {
public:
    FuncStatement(const LexerRange &o,
                  Path p,
                  Type ret,
                  std::vector<VariableStatement *> args,
                  std::vector<Statement *> b,
                  bool var_arg)
        : ScopeStatement(FUNC_STMT, o, std::move(b)), FuncStCommon(std::move(p), ret, std::move(args), var_arg) {}

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
    Path path;
    std::vector<VariableStatement *> members;

    StructStatement(const LexerRange &o, Path p, std::vector<VariableStatement *> m)
        : Statement(STRUCT_STMT, o), path(std::move(p)), members(std::move(m)) {}

    ~StructStatement() override {
        for (auto *m: members) {
            delete m;
        }
    }

    bool has_member(const std::string &n) {
        for (auto mem: members) {
            if (mem->name.raw == n)
                return true;
        }
        return false;
    }

    Type get_type(Path prefix = Path({})) {
        return Type(path.with_prefix(prefix), 0);
    }

    Type get_member_type(const std::string &n) {
        for (auto mem: members) {
            if (mem->name.raw == n)
                return mem->type;
        }
        return {};
    }

    unsigned int get_member_index(const std::string &n) {
        for (size_t i = 0; i < members.size(); i++) {
            if (members[i]->name.raw == n) return i;
        }
        return -1;
    }

    [[nodiscard]] std::string print() const override {
        std::string res = "struct " + path.str() + " {\n";
        for (auto *mem: members) {
            res += mem->print() + "\n";
        }
        return res + "\n}";
    }
};

class ImportStatement : public ScopeStatement {
public:
    std::string name;

    ImportStatement(const LexerRange &o, std::string n, std::vector<Statement *> s)
        : ScopeStatement(IMPORT_STMT, o, std::move(s)), name(std::move(n)) {}
};
} // namespace ast

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_

// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_EXPRESSIONS_STATEMENTS_H_
#define TARIK_SRC_SEMANTIC_EXPRESSIONS_STATEMENTS_H_

#include <map>
#include <utility>
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
        : Statement(t, o),
          block(std::move(b)) {}

    ~ScopeStatement() override {
        for (auto *st : block) {
            delete st;
        }
    }

    static std::string print_statements(std::vector<Statement *> stmts, bool indent = true) {
        std::string res;
        for (auto *st : stmts) {
            std::string t = st->print();
            if (st->statement_type == EXPR_STMT) {
                t.push_back(';');
                std::string prelude = print_statements(((Expression *) st)->collect_prelude(), false);
                if (!prelude.empty())
                    prelude.erase(0, 1);
                t = prelude + t;
            }
            // Add four spaces to the start of every line
            size_t index = 0;
            while (indent) {
                /* Locate the substring to replace. */
                index = t.find('\n', index);
                if (index == std::string::npos)
                    break;

                /* Make the replacement. */
                t.replace(index, 1, "\n    ");

                /* Advance index forward so the next iteration doesn't pick it up as well. */
                index += 3;
            }
            if (indent)
                res += "\n    " + t;
            else
                res += "\n" + t;
        }
        if (!stmts.empty()) {
            res += '\n';
        }
        return res;
    }

    [[nodiscard]] std::string print() const override {
        return "{" + print_statements(block) + "}";
    }
};

class ElseStatement : public ScopeStatement {
public:
    explicit ElseStatement(const LexerRange &o, std::vector<Statement *> block)
        : ScopeStatement(ELSE_STMT, o, std::move(block)) {}

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
        : ScopeStatement(IF_STMT, o, std::move(block)),
          condition(cond) {}

    ~IfStatement() override {
        delete condition;
        delete else_statement;
    }

    [[nodiscard]] std::string print() const override {
        std::string prelude = print_statements(condition->collect_prelude(), false);
        std::string then_string = ScopeStatement::print();
        std::string if_string = prelude + "if " + condition->print() + " " + then_string;
        if (else_statement)
            if_string += " " + else_statement->print();
        return if_string;
    }
};

class ReturnStatement : public Statement {
public:
    // Statement, instead of expression, so we can print it
    Expression *value;

    explicit ReturnStatement(const LexerRange &o, Expression *val)
        : Statement(RETURN_STMT, o),
          value(val) {}

    ~ReturnStatement() override {
        delete value;
    }

    [[nodiscard]] std::string print() const override {
        if (value) {
            std::string prelude = ScopeStatement::print_statements(value->collect_prelude(), false);
            return prelude + "return " + value->print() + ";";
        } else {
            return "return;";
        }
    }
};

class WhileStatement : public ScopeStatement {
public:
    // Statement, instead of expression, so we can print it
    Expression *condition;

    explicit WhileStatement(const LexerRange &o, Expression *cond, std::vector<Statement *> block)
        : ScopeStatement(WHILE_STMT, o, std::move(block)),
          condition(cond) {}

    ~WhileStatement() override {
        delete condition;
    }

    [[nodiscard]] std::string print() const override {
        std::string prelude = print_statements(condition->collect_prelude(), false);
        std::string then_string = ScopeStatement::print();
        return prelude + "while " + condition->print() + " " + then_string;
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
    bool written_to = false;

    VariableStatement(const LexerRange &o, Type t, Token n)
        : Statement(VARIABLE_STMT, o),
          type(std::move(t)),
          name(std::move(n)) {}

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
        : path(std::move(p)),
          return_type(std::move(ret)),
          arguments(std::move(args)),
          var_arg(va) {}

    [[nodiscard]] std::string head() const {
        std::string res = "fn " + path.str() + "(";
        for (auto *arg : arguments) {
            res += arg->type.str() + " " + arg->name.raw + ", ";
        }

        if (var_arg)
            res += "..., ";

        if (res.back() != '(')
            res = res.substr(0, res.size() - 2);
        return res + ") " + return_type.str();
    }

    [[nodiscard]] std::string signature() const {
        std::string res = "(";
        for (auto *arg : arguments) {
            res += arg->type.str() + ", ";
        }

        if (var_arg)
            res += "..., ";

        if (res.back() != '(')
            res = res.substr(0, res.size() - 2);
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
        : Statement(FUNC_DECL_STMT, o),
          FuncStCommon(std::move(p), std::move(ret), std::move(args), var_arg) {}

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
        : ScopeStatement(FUNC_STMT, o, std::move(b)),
          FuncStCommon(std::move(p), std::move(ret), std::move(args), var_arg) {}

    ~FuncStatement() override {
        for (auto *arg : arguments) {
            delete arg;
        }
    }

    [[nodiscard]] std::string print() const override {
        return head() + " " + ScopeStatement::print();
    }
};

class StructDeclareStatement : public Statement {
public:
    Path path;

    StructDeclareStatement(const LexerRange &o, Path p, StmtType type = STRUCT_DECL_STMT)
        : Statement(type, o),
          path(std::move(p)) {}

    Type get_type(Path prefix) const {
        return Type(path.with_prefix(std::move(prefix)), 0);
    }

    [[nodiscard]] std::string print() const override {
        return "struct " + path.str() + ";";
    }
};

class StructStatement : public StructDeclareStatement {
public:
    std::vector<VariableStatement *> members;

    StructStatement(const LexerRange &o, Path p, std::vector<VariableStatement *> m)
        : StructDeclareStatement(o, std::move(p), STRUCT_STMT),
          members(std::move(m)) {}

    ~StructStatement() override {
        for (auto *m : members) {
            delete m;
        }
    }

    bool has_member(const std::string &n) {
        for (auto *mem : members) {
            if (mem->name.raw == n)
                return true;
        }
        return false;
    }

    Type get_member_type(const std::string &n) {
        for (auto *mem : members) {
            if (mem->name.raw == n)
                return mem->type;
        }
        return {};
    }

    unsigned int get_member_index(const std::string &n) {
        for (size_t i = 0; i < members.size(); i++) {
            if (members[i]->name.raw == n)
                return i;
        }
        return -1;
    }

    [[nodiscard]] std::string print() const override {
        std::string res = "struct " + path.str() + " {";
        for (auto *mem : members) {
            res += "\n    " + mem->print();
        }
        return res + "\n}";
    }
};

class ImportStatement : public ScopeStatement {
public:
    Path path;
    bool local;

    ImportStatement(const LexerRange &o, Path p, bool l, std::vector<Statement *> s)
        : ScopeStatement(IMPORT_STMT, o, std::move(s)),
          path(std::move(p)),
          local(l) {}
};
} // namespace ast

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_STATEMENTS_H_

// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_STATEMENTS_H
#define TARIK_STATEMENTS_H

#include <map>
#include <utility>

#include "Types.h"
#include "../Lexer.h"

class Expression;

enum StmtType {
    SCOPE_STMT,
    FUNC_STMT,
    IF_STMT,
    ELSE_STMT,
    RETURN_STMT,
    WHILE_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    VARIABLE_STMT,
    STRUCT_STMT,
    EXPR_STMT
};

class Statement {
public:
    StmtType statement_type{};
    LexerPos origin;

    Statement() = default;

    Statement(const Statement &) = delete;

    Statement(const Statement &&) = delete;

    explicit Statement(StmtType t, LexerPos o) {
        statement_type = t;
        origin = o;
    }

    virtual ~Statement() {};

    [[nodiscard]] virtual std::string print() const = 0;
};

class ScopeStatement : public Statement {
public:
    std::vector<Statement *> block;

    ScopeStatement(StmtType t, LexerPos o, std::vector<Statement *> b)
        : Statement(t, o), block(std::move(b)) {}

    ~ScopeStatement() override {
        for (auto *st : block) {
            delete st;
        }
    }

    [[nodiscard]] std::string print() const override {
        std::string res;
        for (auto *st : block) {
            std::string t = st->print();
            if (st->statement_type == EXPR_STMT)
                t.push_back(';');
            res += t + "\n";
        }
        res.pop_back();
        return res;
    }
};

class IfStatement : public Statement {
public:
    // Statement so we can print it
    Statement *condition;
    Statement *then;

    IfStatement(LexerPos o, Expression *cond, Statement *t)
        : Statement(IF_STMT, o), condition(reinterpret_cast<Statement *>(cond)), then(t) {
    }

    ~IfStatement() override {
        delete condition;
        delete then;
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
    Statement *then;

    explicit ElseStatement(LexerPos o, IfStatement *inv, Statement *t)
        : Statement(ELSE_STMT, o), then(t) {
    }

    ~ElseStatement() override {
        delete then;
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
    Statement *value;

    explicit ReturnStatement(LexerPos o, Expression *val)
        : Statement(RETURN_STMT, o), value(reinterpret_cast<Statement *>(val)) {
    }

    ~ReturnStatement() override {
        delete value;
    }

    [[nodiscard]] std::string print() const override {
        return "return " + value->print() + ";";
    }
};

class WhileStatement : public Statement {
public:
    Statement *condition;
    Statement *then;

    explicit WhileStatement(LexerPos o, Expression *cond, Statement *t)
        : Statement(WHILE_STMT, o), condition(reinterpret_cast<Statement *>(cond)), then(t) {
    }

    ~WhileStatement() override {
        delete condition;
        delete then;
    }

    [[nodiscard]] std::string print() const override {
        std::string then_string = then->print();
        if (then->statement_type == EXPR_STMT)
            then_string += ";";
        return "while " + condition->print() + " {\n" + then_string + "\n}";
    }
};

class BreakStatement : public Statement {
public:
    explicit BreakStatement(LexerPos o)
        : Statement(BREAK_STMT, o) {}

    [[nodiscard]] std::string print() const override {
        return "break;";
    }
};

class ContinueStatement : public Statement {
public:
    explicit ContinueStatement(LexerPos o)
        : Statement(CONTINUE_STMT, o) {}

    [[nodiscard]] std::string print() const override {
        return "continue;";
    }
};

class VariableStatement : public Statement {
public:
    Type type;
    std::string name;

    VariableStatement(LexerPos o, Type t, std::string n)
        : Statement(VARIABLE_STMT, o), type(t), name(std::move(n)) {}

    [[nodiscard]] std::string print() const override {
        return "<todo: type to string> " + name + ";";
    }
};

class FuncStatement : public ScopeStatement {
public:
    std::string name;
    Type return_type;
    std::vector<VariableStatement *> arguments;

    FuncStatement(LexerPos o, std::string n, Type ret, std::vector<VariableStatement *> args, std::vector<Statement *> b)
        : ScopeStatement(FUNC_STMT,
                         o,
                         std::move(b)), name(std::move(n)), return_type(ret), arguments(std::move(args)) {}

    ~FuncStatement() override {
        ScopeStatement::~ScopeStatement();
        for (auto *arg : arguments) {
            delete arg;
        }
    }

    [[nodiscard]] std::string print() const override {
        std::string res = "fn " + name + "(";
        for (auto arg : arguments) {
            res += "<todo: type to string> , ";
        }
        if (res.back() != '(')
            res = res.substr(0, res.size() - 2);
        return res + ") <todo: type to string> {\n" + ScopeStatement::print() + "\n}";
    }
};

class StructStatement : public Statement {
public:
    std::string name;
    std::vector<VariableStatement *> members;

    ~StructStatement() override {
        for (auto *m : members) {
            delete m;
        }
    }

    bool has_member(std::string name) {
        return std::any_of(members.front(), members.back(), [name](const VariableStatement &mem) {
            return mem.name == name;
        });
    }

    Type get_member_type(std::string name) {
        for (auto *mem : members) {
            if (mem->name == name)
                return mem->type;
        }
        return {};
    }

    [[nodiscard]] std::string print() const override {
        std::string res = "struct " + name + " {\n";
        for (auto *mem : members) {
            res += mem->print() + "\n";
        }
        return res + "\n}";
    }
};

#endif //TARIK_STATEMENTS_H

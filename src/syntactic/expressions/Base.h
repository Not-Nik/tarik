// tarik (c) Nikolas Wipper 2021-2023

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_BASE_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_BASE_H_

#include <string>

#include "lexical/Lexer.h"

enum StmtType {
    SCOPE_STMT,
    FUNC_STMT,
    FUNC_DECL_STMT,
    IF_STMT,
    ELSE_STMT,
    RETURN_STMT,
    WHILE_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    VARIABLE_STMT,
    STRUCT_STMT,
    IMPORT_STMT,
    EXPR_STMT
};

class Statement {
public:
    StmtType statement_type {};
    [[maybe_unused]] LexerPos origin {};

    Statement() = default;

    Statement(const Statement &) = delete;

    Statement(const Statement &&) = delete;

    explicit Statement(StmtType t, const LexerPos &o)
        : origin(o) {
        statement_type = t;
        origin = o;
    }

    virtual ~Statement() = default;

    [[nodiscard]] virtual std::string print() const = 0;
};

enum Precedence {
    ASSIGNMENT = 1,
    EQUALITY,
    COMPARE,
    SUM,
    PRODUCT,
    PREFIX,
    CALL
};

enum ExprType {
    CALL_EXPR,
    DASH_EXPR,
    DOT_EXPR,
    EQ_EXPR,
    COMP_EXPR,
    MEM_ACC_EXPR,
    PREFIX_EXPR,
    ASSIGN_EXPR,
    NAME_EXPR,
    INT_EXPR,
    BOOL_EXPR,
    REAL_EXPR,
    STR_EXPR,
    NULL_EXPR
};

class Expression : public Statement {
public:
    ExprType expression_type;
    Type type = {};

    Expression()
        : Statement(),
          expression_type(NAME_EXPR) {
    }

    explicit Expression(ExprType t, const LexerPos &lp) : Statement(EXPR_STMT, lp) { expression_type = t; }

    void assign_type(Type t) { type = t; }
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_BASE_H_

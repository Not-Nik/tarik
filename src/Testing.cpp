// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Testing.h"

#include "syntactic/Parser.h"
#include "syntactic/Types.h"
#include "syntactic/ast/Expression.h"

#include <sstream>

void *operator new(size_t size) {
    void *p = malloc(size);
    allocs++;
    return p;
}

void operator delete(void *p) noexcept {
    free(p);
    allocs--;
}

// this is disgusting
namespace std {
std::string to_string(const ast::FuncStatement *f) {
    return f->head();
}

inline std::string to_string(const Type &t) {
    return t.str();
}
}

bool test() {
    using ss = std::stringstream;
    int overhead = allocs;
    Bucket error_bucket;
    BEGIN_TEST

    FIRST_TEST(utility)

        ASSERT_STR_EQ(Type(Path({"ÜberÄnderung"})).func_name(), "über_änderung")

    MID_TEST(lexer)

        ss c("hello under_score test4 4test ( ) +-===- > fn i32 42 12.34 . ... \"a string\"# comment should be ignored\nback");
        Lexer lexer(&c);

        ASSERT_TOK(NAME, "hello")
        ASSERT_STR_EQ(lexer.peek(1).raw, "test4")
        ASSERT_TOK(NAME, "under_score")
        ASSERT_TOK(NAME, "test4")
        ASSERT_TOK(INTEGER, "4")
        ASSERT_TOK(NAME, "test")
        ASSERT_TOK(PAREN_OPEN, "(")
        ASSERT_TOK(PAREN_CLOSE, ")")
        ASSERT_TOK(PLUS, "+")
        ASSERT_TOK(MINUS, "-")
        ASSERT_TOK(DOUBLE_EQUAL, "==")
        ASSERT_TOK(EQUAL, "=")
        ASSERT_TOK(MINUS, "-")
        ASSERT_TOK(GREATER, ">")
        ASSERT_TOK(FUNC, "fn")
        ASSERT_TOK(TYPE, "i32")
        ASSERT_TOK(INTEGER, "42")
        ASSERT_TOK(REAL, "12.34")
        ASSERT_TOK(PERIOD, ".")
        ASSERT_TOK(TRIPLE_PERIOD, "...")
        ASSERT_TOK(STRING, "a string")
        ASSERT_TOK(NAME, "back")//

    MID_TEST(expression parsing)

        ss c; {
            ast::Expression *e = Parser(&(c = ss("3 + 4 * 5")), &error_bucket).parse_expression();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_STR_EQ(e->print(), "(3+(4*5))")
            delete e;
        } {
            ast::Expression *e = Parser(&(c = ss("-3 + -4 * 5")), &error_bucket).parse_expression();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_STR_EQ(e->print(), "(-3+(-4*5))")
            delete e;
        } {
            Parser p(&(c = ss("-name + 4 * -5")), &error_bucket);
            ASSERT_NO_ERROR(error_bucket)
            ast::Expression *e = p.parse_expression();
            ASSERT_STR_EQ(e->print(), "(-name+(4*-5))")
            delete e;
        } {
            ast::Expression *e = Parser(&(c = ss("(3 + 4) * 5")), &error_bucket).parse_expression();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_STR_EQ(e->print(), "((3+4)*5)")
            delete e;
        } {
            Parser tmp = Parser(&(c = ss("3 + 4 * 5; 6 + 7 * 8")), &error_bucket);
            ast::Expression *e = tmp.parse_expression();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_STR_EQ(e->print(), "(3+(4*5))")
            delete e;
            tmp.expect(SEMICOLON);
            e = tmp.parse_expression();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_STR_EQ(e->print(), "(6+(7*8))")
            delete e;
        } {
            Parser p = Parser(&(c = ss("func(1, 2, 3, 4)")), &error_bucket);
            ast::Expression *e = p.parse_expression();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_STR_EQ(e->print(), "func(1, 2, 3, 4)")
            delete e;
        }//

    MID_TEST(statement parsing)

        ss c;

        // If/while (they're syntactically the same)
        ast::IfStatement *ifStatement = (ast::IfStatement *) Parser(&(c = ss("if 4 + 4 {}")), &error_bucket).parse_statement();
            ASSERT_NO_ERROR(error_bucket)
        ASSERT_EQ(ifStatement->statement_type, ast::IF_STMT)
        ASSERT_STR_EQ(ifStatement->condition->print(), "(4+4)")
        delete ifStatement;

        // Functions
        {
            Parser p(&(c = ss("fn test_func(i32 arg1, f64 arg2) i8 {} test_func(1, 2.3);")), &error_bucket);
            auto *func = (ast::FuncStatement *) p.parse_statement();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_EQ(func->statement_type, ast::FUNC_STMT)
            ASSERT_STR_EQ(func->name.raw, "test_func")
            ASSERT_TRUE(func->return_type == Type(I8))
            ASSERT_STR_EQ(func->arguments[0]->name.raw, "arg1")
            ASSERT_EQ(func->arguments[0]->type, Type(I32))
            ASSERT_STR_EQ(func->arguments[1]->name.raw, "arg2")
            ASSERT_EQ(func->arguments[1]->type, Type(F64))
            ASSERT_EQ(func->return_type, Type(I8))

            auto *call = (ast::CallExpression *) p.parse_statement();
            ASSERT_NO_ERROR(error_bucket)
            ASSERT_STR_EQ(call->callee->print(), "test_func")

            delete call;
            delete func;
        }

        // Variables and assignments
        Parser p(&(c = ss("u8 test_var = 4; test_var = 5; u32 *test_ptr;")), &error_bucket);
        auto *var = (ast::VariableStatement *) p.parse_statement();
        ASSERT_NO_ERROR(error_bucket)
        ASSERT_EQ(var->statement_type, ast::VARIABLE_STMT)
        ASSERT_STR_EQ(var->name.raw, "test_var")
        ASSERT_TRUE(var->type.is_primitive())
        ASSERT_EQ(var->type.pointer_level, 0)
        ASSERT_EQ(var->type, Type(U8))

        auto *first = (ast::Expression *) p.parse_statement(), *second = (ast::Expression *) p.parse_statement();
        ASSERT_NO_ERROR(error_bucket)
        ASSERT_EQ(first->expression_type, ast::ASSIGN_EXPR)
        ASSERT_EQ(second->expression_type, ast::ASSIGN_EXPR)
        delete var;
        delete first;
        delete second;

        auto *ptr = (ast::VariableStatement *) p.parse_statement();
        ASSERT_EQ(ptr->statement_type, ast::VARIABLE_STMT)
        ASSERT_STR_EQ(ptr->name.raw, "test_ptr")
        ASSERT_TRUE(ptr->type.is_primitive())
        ASSERT_EQ(ptr->type.pointer_level, 1)
        ASSERT_EQ(ptr->type, Type(U32, 1))
        delete ptr;//

    MID_TEST(full)

        ss c = ss("fn main() u8 {\n" "  i32 some_int = 4 + 5 * 3 / 6 - 2;\n" "  some_int = some_int + 7;\n"
                  "  if some_int {\n" "      some_int = 0;\n" "  }\n" "  return some_int;\n" "}");

        Parser p(&c, &error_bucket);
        ast::Statement *s = p.parse_statement();
        ASSERT_NO_ERROR(error_bucket)

        ASSERT_EQ(s->statement_type, ast::FUNC_STMT)

        auto *f = (ast::FuncStatement *) s;

        ASSERT_STR_EQ(f->name.raw, "main")
        ASSERT_TRUE(f->return_type == Type(U8))

        ASSERT_TRUE(f->arguments.empty())

        ASSERT_EQ(f->block.size(), 5)
        ASSERT_EQ(f->block[0]->statement_type, ast::VARIABLE_STMT)
        ASSERT_EQ(f->block[1]->statement_type, ast::EXPR_STMT)
        ASSERT_EQ(f->block[2]->statement_type, ast::EXPR_STMT)
        ASSERT_EQ(f->block[3]->statement_type, ast::IF_STMT)
        ASSERT_EQ(f->block[4]->statement_type, ast::RETURN_STMT)

        auto *if_stmt = (ast::IfStatement *) f->block[3];

        ASSERT_EQ(if_stmt->condition->statement_type, ast::EXPR_STMT)

        auto *if_cond = (ast::Expression *) if_stmt->condition;

        ASSERT_EQ(if_cond->expression_type, ast::NAME_EXPR)

        ASSERT_EQ(if_stmt->block.size(), 1)
        ASSERT_EQ(if_stmt->block[0]->statement_type, ast::EXPR_STMT)

        auto *ret_stmt = (ast::ReturnStatement *) f->block[4];

        ASSERT_EQ(ret_stmt->value->statement_type, ast::EXPR_STMT)
        ASSERT_EQ(((ast::Expression *) ret_stmt->value)->expression_type, ast::NAME_EXPR)

        delete s; //

    MID_TEST(memory management)

        ASSERT_EQ(allocs - overhead, 0)//

    END_TEST
}

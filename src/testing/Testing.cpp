// tarik (c) Nikolas Wipper 2021-2025

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
namespace std
{
std::string to_string(const ast::FuncStatement *f) {
    return f->head();
}

inline std::string to_string(const Type &t) {
    return t.str();
}
}

void Tester::StartSegment(const std::string &name) {
    count_suc = 0;
    count_tested = 0;

    std::print("Testing {}...", name);
}

void Tester::EndSegment() const {
    printf(" Done! (%i/%i succeeded)\n", count_suc, count_tested);
}

bool Tester::Assert(bool condition, std::string msg) {
    count_tested++;
    if (!condition) {
        std::print("\n{}", msg);
    } else {
        count_suc++;
    }

    st = st && condition;
    return condition;
}

void Tester::AssertTok(Lexer lexer, TokenType type, const std::string &tok) {
    std::string s = lexer.peek().raw;
    std::string &sr = s;
    Assert(lexer.peek().raw == tok, std::format("Failed for token '{}': expected '{}'.", sr, tok));
    lexer.consume();
}

void Tester::AssertTrue(bool condition) {
    Assert(condition, "Failed");
}

void Tester::AssertNoError(Bucket &bucket) {
    if (!Assert(bucket.get_error_count() == 0, "Failed to parse expression.\n"))
        bucket.print_errors();
}


void selftest(Tester &tester) {
    using ss = std::stringstream;
    int overhead = allocs;
    Bucket error_bucket;

    tester.StartSegment("utility");
    {
        tester.AssertEq(Type(Path({"ÜberÄnderung"})).func_name(), "über_änderung");
    }

    tester.EndSegment();
    tester.StartSegment("lexer");

    {
        ss c(
            "hello under_score test4 4test ( ) +-===- > fn i32 42 12.34 . ... \"a string\"# comment should be ignored\nback");
        Lexer lexer(&c);

        tester.AssertTok(lexer, NAME, "hello");
        tester.AssertEq(lexer.peek(1).raw, "test4");
        tester.AssertTok(lexer, NAME, "under_score");
        tester.AssertTok(lexer, NAME, "test4");
        tester.AssertTok(lexer, INTEGER, "4");
        tester.AssertTok(lexer, NAME, "test");
        tester.AssertTok(lexer, PAREN_OPEN, "(");
        tester.AssertTok(lexer, PAREN_CLOSE, ")");
        tester.AssertTok(lexer, PLUS, "+");
        tester.AssertTok(lexer, MINUS, "-");
        tester.AssertTok(lexer, DOUBLE_EQUAL, "==");
        tester.AssertTok(lexer, EQUAL, "=");
        tester.AssertTok(lexer, MINUS, "-");
        tester.AssertTok(lexer, GREATER, ">");
        tester.AssertTok(lexer, FUNC, "fn");
        tester.AssertTok(lexer, TYPE, "i32");
        tester.AssertTok(lexer, INTEGER, "42");
        tester.AssertTok(lexer, REAL, "12.34");
        tester.AssertTok(lexer, PERIOD, ".");
        tester.AssertTok(lexer, TRIPLE_PERIOD, "...");
        tester.AssertTok(lexer, STRING, "a string");
        tester.AssertTok(lexer, NAME, "back");
    }

    tester.EndSegment();
    tester.StartSegment("expression parsing");

    {
        ss c;
        {
            ast::Expression *e = Parser(&(c = ss("3 + 4 * 5")), &error_bucket).parse_expression();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(e->print(), "(3+(4*5))");
            delete e;
        }
        {
            ast::Expression *e = Parser(&(c = ss("-3 + -4 * 5")), &error_bucket).parse_expression();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(e->print(), "(-3+(-4*5))");
            delete e;
        }
        {
            Parser p(&(c = ss("-name + 4 * -5")), &error_bucket);
            tester.AssertNoError(error_bucket);
            ast::Expression *e = p.parse_expression();
            tester.AssertEq(e->print(), "(-name+(4*-5))");
            delete e;
        }
        {
            ast::Expression *e = Parser(&(c = ss("(3 + 4) * 5")), &error_bucket).parse_expression();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(e->print(), "((3+4)*5)");
            delete e;
        }
        {
            Parser tmp = Parser(&(c = ss("3 + 4 * 5; 6 + 7 * 8")), &error_bucket);
            ast::Expression *e = tmp.parse_expression();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(e->print(), "(3+(4*5))");
            delete e;
            tmp.expect(SEMICOLON);
            e = tmp.parse_expression();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(e->print(), "(6+(7*8))");
            delete e;
        }
        {
            Parser p = Parser(&(c = ss("func(1, 2, 3, 4)")), &error_bucket);
            ast::Expression *e = p.parse_expression();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(e->print(), "func(1, 2, 3, 4)");
            delete e;
        }
    }

    tester.EndSegment();
    tester.StartSegment("statement parsing");

    {
        ss c;

        // If/while (they're syntactically the same)
        ast::IfStatement *ifStatement = (ast::IfStatement *) Parser(&(c = ss("if 4 + 4 {}")), &error_bucket).
                parse_statement();
        tester.AssertNoError(error_bucket);
        tester.AssertEq(ifStatement->statement_type, ast::IF_STMT);
        tester.AssertEq(ifStatement->condition->print(), "(4+4)");
        delete ifStatement;

        // Functions
        {
            Parser p(&(c = ss("fn test_func(i32 arg1, f64 arg2) i8 {} test_func(1, 2.3);")), &error_bucket);
            auto *func = (ast::FuncStatement *) p.parse_statement();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(func->statement_type, ast::FUNC_STMT);
            tester.AssertEq(func->name.raw, "test_func");
            tester.AssertEq(func->return_type, Type(I8));
            tester.AssertEq(func->arguments[0]->name.raw, "arg1");
            tester.AssertEq(func->arguments[0]->type, Type(I32));
            tester.AssertEq(func->arguments[1]->name.raw, "arg2");
            tester.AssertEq(func->arguments[1]->type, Type(F64));
            tester.AssertEq(func->return_type, Type(I8));

            auto *call = (ast::CallExpression *) p.parse_statement();
            tester.AssertNoError(error_bucket);
            tester.AssertEq(call->callee->print(), "test_func");

            delete call;
            delete func;
        }

        // Variables and assignments
        Parser p(&(c = ss("u8 test_var = 4; test_var = 5; u32 *test_ptr;")), &error_bucket);
        auto *var = (ast::VariableStatement *) p.parse_statement();
        tester.AssertNoError(error_bucket);
        tester.AssertEq(var->statement_type, ast::VARIABLE_STMT);
        tester.AssertEq(var->name.raw, "test_var");
        tester.AssertTrue(var->type.is_primitive());
        tester.AssertEq(var->type.pointer_level, 0);
        tester.AssertEq(var->type, Type(U8));

        auto *first = (ast::Expression *) p.parse_statement(), *second = (ast::Expression *) p.parse_statement();
        tester.AssertNoError(error_bucket);
        tester.AssertEq(first->expression_type, ast::ASSIGN_EXPR);
        tester.AssertEq(second->expression_type, ast::ASSIGN_EXPR);
        delete var;
        delete first;
        delete second;

        auto *ptr = (ast::VariableStatement *) p.parse_statement();
        tester.AssertEq(ptr->statement_type, ast::VARIABLE_STMT);
        tester.AssertEq(ptr->name.raw, "test_ptr");
        tester.AssertTrue(ptr->type.is_primitive());
        tester.AssertEq(ptr->type.pointer_level, 1);
        tester.AssertEq(ptr->type, Type(U32, 1));
        delete ptr;
    }

    tester.EndSegment();
    tester.StartSegment("full");

    {
        ss c = ss("fn main() u8 {\n" "  i32 some_int = 4 + 5 * 3 / 6 - 2;\n" "  some_int = some_int + 7;\n"
            "  if some_int {\n" "      some_int = 0;\n" "  }\n" "  return some_int;\n" "}");

        Parser p(&c, &error_bucket);
        ast::Statement *s = p.parse_statement();
        tester.AssertNoError(error_bucket);

        tester.AssertEq(s->statement_type, ast::FUNC_STMT);

        auto *f = (ast::FuncStatement *) s;

        tester.AssertEq(f->name.raw, "main");
        tester.AssertEq(f->return_type, Type(U8));

        tester.AssertTrue(f->arguments.empty());

        tester.AssertEq(f->block.size(), 5);
        tester.AssertEq(f->block[0]->statement_type, ast::VARIABLE_STMT);
        tester.AssertEq(f->block[1]->statement_type, ast::EXPR_STMT);
        tester.AssertEq(f->block[2]->statement_type, ast::EXPR_STMT);
        tester.AssertEq(f->block[3]->statement_type, ast::IF_STMT);
        tester.AssertEq(f->block[4]->statement_type, ast::RETURN_STMT);

        auto *if_stmt = (ast::IfStatement *) f->block[3];

        tester.AssertEq(if_stmt->condition->statement_type, ast::EXPR_STMT);

        auto *if_cond = (ast::Expression *) if_stmt->condition;

        tester.AssertEq(if_cond->expression_type, ast::NAME_EXPR);

        tester.AssertEq(if_stmt->block.size(), 1);
        tester.AssertEq(if_stmt->block[0]->statement_type, ast::EXPR_STMT);

        auto *ret_stmt = (ast::ReturnStatement *) f->block[4];

        tester.AssertEq(ret_stmt->value->statement_type, ast::EXPR_STMT);
        tester.AssertEq(((ast::Expression *) ret_stmt->value)->expression_type, ast::NAME_EXPR);

        delete s;
    }

    tester.EndSegment();
    tester.StartSegment("memory management");

    {
        tester.AssertEq(allocs - overhead, 0);
    }
    tester.EndSegment();
}

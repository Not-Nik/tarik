// tarik (c) Nikolas Wipper 2021

#include "Testing.h"

#include "Parser.h"
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

namespace std {
std::string to_string(FuncStatement *f) {
    return f->head();
}
}

bool test() {
    using ss = std::stringstream;
    int overhead = allocs;
    Type integer = Type(TypeUnion{I64}, true, 0);
    BEGIN_TEST

    FIRST_TEST(lexer)

        ss c("hello under_score test4 4test ( ) +-===- > fn i32 42 12.34 \"a string\"# comment should be ignored\nback");
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
        ASSERT_TOK(STRING, "\"a string\"")
        ASSERT_TOK(NAME, "back")MID_TEST(expression parsing)

        ss c;
        {
            Expression *e = Parser(&(c = ss("3 + 4 * 5"))).parse_expression();
            ASSERT_STR_EQ(e->print(), "(3+(4*5))")
            delete e;
        }
        {
            Expression *e = Parser(&(c = ss("-3 + -4 * 5"))).parse_expression();
            ASSERT_STR_EQ(e->print(), "(-3+(-4*5))")
            delete e;
        }
        {
            Parser p(&(c = ss("-name + 4 * -5")));
            auto *v = p.register_var(new VariableStatement({}, integer, "name"));
            Expression *e = p.parse_expression();
            ASSERT_STR_EQ(e->print(), "(-name+(4*-5))")
            delete v;
            delete e;
        }
        {
            Expression *e = Parser(&(c = ss("(3 + 4) * 5"))).parse_expression();
            ASSERT_STR_EQ(e->print(), "((3+4)*5)")
            delete e;
        }
        {
            Parser tmp = Parser(&(c = ss("3 + 4 * 5; 6 + 7 * 8")));
            Expression *e = tmp.parse_expression();
            ASSERT_STR_EQ(e->print(), "(3+(4*5))")
            delete e;
            tmp.expect(SEMICOLON);
            e = tmp.parse_expression();
            ASSERT_STR_EQ(e->print(), "(6+(7*8))")
            delete e;
        }

        {
            Parser p = Parser(&(c = ss("func(1, 2, 3, 4)")));
            FuncStatement *f = p.register_func(new FuncStatement({}, "func", {}, {
                new VariableStatement({}, integer, ""), new VariableStatement({}, integer, ""), new VariableStatement({}, integer, ""),
                new VariableStatement({}, integer, "")
            }, {}));
            Expression *e = p.parse_expression();
            ASSERT_STR_EQ(e->print(), "func(1, 2, 3, 4)")
            delete f;
            delete e;
        }MID_TEST(statement parsing)

        ss c;

        // If/while (they're syntactically the same)
        IfStatement *ifStatement = (IfStatement *) Parser(&(c = ss("if 4 + 4 {}"))).parse_statement(false);
        ASSERT_EQ(ifStatement->statement_type, IF_STMT)
        ASSERT_STR_EQ(ifStatement->condition->print(), "(4+4)")
        delete ifStatement;

        // Functions
        {
            Parser p(&(c = ss("fn test_func(i32 arg1, f64 arg2) i8 {} test_func(1, 2.3);")));
            auto func = (FuncStatement *) p.parse_statement();
            ASSERT_EQ(func->statement_type, FUNC_STMT)
            ASSERT_STR_EQ(func->name, "test_func")
            ASSERT_TRUE(func->return_type == Type(I8))
            ASSERT_STR_EQ(func->arguments[0]->name, "arg1")
            ASSERT_EQ(func->arguments[0]->type.type.size, I32)
            ASSERT_STR_EQ(func->arguments[1]->name, "arg2")
            ASSERT_EQ(func->arguments[1]->type.type.size, F64)
            ASSERT_EQ(func->return_type.type.size, I8)

            auto call = (CallExpression *) p.parse_statement(false);
            ASSERT_EQ(call->function, func);

            delete call;
            delete func;
        }

        // Variables and assignments
        Parser p(&(c = ss("u8 test_var = 4; test_var = 5; u32 *test_ptr;")));
        auto *var = (VariableStatement *) p.parse_statement();
        ASSERT_EQ(var->statement_type, VARIABLE_STMT)
        ASSERT_STR_EQ(var->name, "test_var")
        ASSERT_TRUE(var->type.is_primitive)
        ASSERT_EQ(var->type.pointer_level, 0)
        ASSERT_EQ(var->type.type.size, U8)

        auto *first = (Expression *) p.parse_statement(false), *second = (Expression *) p.parse_statement(false);
        ASSERT_EQ(first->expression_type, ASSIGN_EXPR)
        ASSERT_EQ(second->expression_type, ASSIGN_EXPR)
        delete var;
        delete first;
        delete second;

        auto *ptr = (VariableStatement *) p.parse_statement();
        ASSERT_EQ(ptr->statement_type, VARIABLE_STMT)
        ASSERT_STR_EQ(ptr->name, "test_ptr")
        ASSERT_TRUE(ptr->type.is_primitive)
        ASSERT_EQ(ptr->type.pointer_level, 1)
        ASSERT_EQ(ptr->type.type.size, U32)
        delete ptr; MID_TEST(full)

        ss c = ss("fn main() u8 {\n"
                  "  i32 some_int = 4 + 5 * 3 / 6 - 2;\n"
                  "  some_int = some_int + 7;\n"
                  "  if some_int {\n"
                  "      some_int = 0;\n"
                  "  }\n"
                  "  return some_int;\n"
                  "}");

        Parser p(&c);
        Statement *s = p.parse_statement();

        ASSERT_EQ(s->statement_type, FUNC_STMT)

        auto *f = (FuncStatement *) s;

        ASSERT_STR_EQ(f->name, "main")
        ASSERT_TRUE(f->return_type == Type(U8))

        ASSERT_TRUE(f->arguments.empty())

        ASSERT_EQ(f->block.size(), 5)
        ASSERT_EQ(f->block[0]->statement_type, VARIABLE_STMT)
        ASSERT_EQ(f->block[1]->statement_type, EXPR_STMT)
        ASSERT_EQ(f->block[2]->statement_type, EXPR_STMT)
        ASSERT_EQ(f->block[3]->statement_type, IF_STMT)
        ASSERT_EQ(f->block[4]->statement_type, RETURN_STMT)

        auto *if_stmt = (IfStatement *) f->block[3];

        ASSERT_EQ(if_stmt->then->statement_type, SCOPE_STMT)
        ASSERT_EQ(if_stmt->condition->statement_type, EXPR_STMT)

        auto if_cond = (Expression *) if_stmt->condition;

        ASSERT_EQ(if_cond->expression_type, VARREF_EXPR)

        auto if_scope = (ScopeStatement *) if_stmt->then;

        ASSERT_EQ(if_scope->block.size(), 1)
        ASSERT_EQ(if_scope->block[0]->statement_type, EXPR_STMT)

        auto *ret_stmt = (ReturnStatement *) f->block[4];

        ASSERT_EQ(ret_stmt->value->statement_type, EXPR_STMT)
        ASSERT_EQ(((Expression *) ret_stmt->value)->expression_type, VARREF_EXPR)

        delete s; MID_TEST(memory management)

        ASSERT_EQ(allocs - overhead, 0)END_TEST
}

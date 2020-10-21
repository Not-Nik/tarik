// tarik (c) Nikolas Wipper 2020

#include "Parser.h"
#include "cli/arguments.h"

#include <iostream>

#define TEST_OPTION 1

static int allocs = 0;

void * operator new(size_t size) {
    void * p = malloc(size);
    allocs++;
    return p;
}

void operator delete(void * p) {
    free(p);
    allocs--;
}

bool test();

int main(int argc, const char * argv[]) {
    /*
    Parser p("fn main() u8 {\n"
             "  i32 some_int = 4 + 5 * 3 / 6 - 2;\n"
             "  some_int = some_int + 7;\n"
             "  if some_int\n"
             "      some_int = 0;\n"
             "  return some_int;\n"
             "}");
    Statement * s = p.parse_statement();
    std::cout << s->print() << std::endl;
    delete s;
    */

    ArgumentParser parser(argc, argv, "tarik");
    parser.addOption(Option {
            .name = "test",
            .description = "Run internal tarik tests",
            .optionID = TEST_OPTION,
            .hasArg = false
    });
    ParsedOption option;
    while ((option = parser.parseNextArg()).option) {
        if (option.option->optionID == TEST_OPTION) {
            int r = test();
            if (r)
                puts("Test succeeded");
            else
                puts("Test failed");
            return !r;
        }
    }
    return 0;
}

#define BEGIN_TEST bool _st = true; int count_suc = 0; int count_tested = 0;
#define FIRST_TEST(name) for (int i = 0; i < 1; i++) { printf("testing %s...", #name);
#define MID_TEST(name) } printf(" done (%i/%i succeeded)\n", count_suc, count_tested); \
count_suc = 0; count_tested = 0;\
for (int i = 0; i < 1; i++) { printf("testing %s...", #name);
#define END_TEST } printf(" done (%i/%i succeeded)\n", count_suc, count_tested); return _st;

#define ASSERT_TOK(type, tok) {_st = lexer.peek().id == type;\
_st = _st && lexer.peek().raw == tok;\
count_tested++;\
if (!_st) { printf("\nFailed for token '%s': expected '%s'", lexer.peek().raw.c_str(), tok); break; }\
count_suc++;\
lexer.consume();}

#define ASSERT_STR_EQ(l, r) {std::string save = l;\
_st = save == r;\
count_tested++;\
if (!_st) { printf("\nFailed for expression '%s': expected '%s'", save.c_str(), r); break; }}\
count_suc++;

#define ASSERT_EQ(l, r) {_st = l == r;\
count_tested++;\
if (!_st) { printf("\nFailed for expression '%s': expected '%s'", #l, #r); break; }\
count_suc++;}

#define ASSERT_TRUE(e) {_st = e; \
count_tested++;\
if (!_st) { printf("\nFailed for expression: expected '%s' to be true", #e); return false; }\
count_suc++;}

bool test() {
    int overhead = allocs;
    BEGIN_TEST

    FIRST_TEST(lexer)
        Lexer lexer("hello under_score test4 4test ( ) +-===- > fn i32 42 12.34 \"a string\"# comment should be ignored\nback");

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
        ASSERT_TOK(NAME, "back")

    MID_TEST(expression parsing)

        Expression * e = Parser("3 + 4 * 5").parse_expression();
        ASSERT_STR_EQ(e->print(), "(3+(4*5))")
        delete e;
        e = Parser("-3 + -4 * 5").parse_expression();
        ASSERT_STR_EQ(e->print(), "(-3+(-4*5))")
        delete e;
        e = Parser("-name + 4 * -5").variable_less()->parse_expression();
        ASSERT_STR_EQ(e->print(), "(-name+(4*-5))")
        delete e;
        e = Parser("(3 + 4) * 5").parse_expression();
        ASSERT_STR_EQ(e->print(), "((3+4)*5)")
        delete e;
        {
            Parser tmp = Parser("3 + 4 * 5; 6 + 7 * 8");
            e = tmp.parse_expression();
            ASSERT_STR_EQ(e->print(), "(3+(4*5))")
            delete e;
            tmp.expect(SEMICOLON);
            e = tmp.parse_expression();
            ASSERT_STR_EQ(e->print(), "(6+(7*8))")
            delete e;
        }

    MID_TEST(statement parsing)

        // If/while (they're syntactically the same)
        IfStatement * ifStatement = (IfStatement *) Parser("if 4 + 4 {}").parse_statement();
        ASSERT_EQ(ifStatement->statement_type, IF_STMT)
        ASSERT_STR_EQ(ifStatement->condition->print(), "(4+4)")
        delete ifStatement;

        // Functions
        FuncStatement * func = (FuncStatement *) Parser("fn test_func(i32 arg1, f64 arg2) i8 {}").parse_statement();
        ASSERT_EQ(func->statement_type, FUNC_STMT)
        ASSERT_STR_EQ(func->name, "test_func")
        ASSERT_EQ(func->arguments["arg1"].type.size, I32)
        ASSERT_EQ(func->arguments["arg2"].type.size, F64)
        ASSERT_EQ(func->return_type.type.size, I8)
        delete func;

        // Variables and assignments
        Parser p("u8 test_var = 4; test_var = 5;");
        auto * var = (VariableStatement *) p.parse_statement();
        ASSERT_EQ(var->statement_type, VARIABLE_STMT)
        ASSERT_STR_EQ(var->name, "test_var")
        ASSERT_TRUE(var->type.is_primitive)
        ASSERT_EQ(var->type.pointer_level, 0)
        ASSERT_EQ(var->type.type.size, U8)
        delete var;

        auto * first = (Expression *) p.parse_statement(), * second = (Expression *) p.parse_statement();
        ASSERT_EQ(first->expression_type, ASSIGN_EXPR)
        ASSERT_EQ(second->expression_type, ASSIGN_EXPR)
        delete first;
        delete second;

    MID_TEST(memory management)

        ASSERT_EQ(allocs - overhead, 0)

    END_TEST
}

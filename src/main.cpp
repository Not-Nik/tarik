// Seno (c) Nikolas Wipper 2020

#include "Parser.h"
#include "cli/arguments.h"

#define TEST_OPTION 1

bool test();

int main(int argc, const char * argv[]) {
    ArgumentParser parser(argc, argv, "Seno");
    parser.addOption(Option {
            .name = "test",
            .description = "Run internal Seno tests",
            .optionID = TEST_OPTION,
            .hasArg = false
    });
    ParsedOption option;
    while ((option = parser.parseNextArg()).option) {
        if (option.option->optionID == TEST_OPTION) {
            if (test())
                puts("Test succeeded");
            else
                puts("Test failed");
        }
    }
    return 0;
}

#define BEGIN_TEST bool _st = true
#define END_TEST return _st

#define ASSERT_TOK(type, tok) _st = _st && lexer.peek().id == type; \
_st = _st && lexer.peek().raw == tok;\
if (!_st) { printf("\nFailed for token '%s': expected '%s'", lexer.peek().raw.c_str(), tok); return false; }\
lexer.consume();

#define ASSERT_STR_EQ(l, r) {std::string save = l; \
_st = save == r; \
if (!_st) { printf("\nFailed for expression '%s': expected '%s'", save.c_str(), r); return false; }}

#define ASSERT_EQ(l, r) _st = l == r; \
if (!_st) { printf("\nFailed for expression '%s': expected '%s'", #l, #r); return false; }

bool test() {
    BEGIN_TEST;

    printf("testing lexer...");
    Lexer lexer("hello test4 4test ( ) +-===- > fn i32 42 12.34 \"a string\"# comment should be ignored\nback");

    ASSERT_TOK(NAME, "hello")
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
    ASSERT_TOK(INT_32, "i32")
    ASSERT_TOK(INTEGER, "42")
    ASSERT_TOK(REAL, "12.34")
    ASSERT_TOK(STRING, "\"a string\"")
    ASSERT_TOK(NAME, "back")

    puts(" done");

    printf("testing expression parsing...");

    ASSERT_STR_EQ(Parser("3 + 4 * 5").parse_expression()->print(), "(3+(4*5))")
    ASSERT_STR_EQ(Parser("-3 + -4 * 5").parse_expression()->print(), "(-3+(-4*5))")
    ASSERT_STR_EQ(Parser("-name + 4 * -5").parse_expression()->print(), "(-name+(4*-5))")
    ASSERT_STR_EQ(Parser("(3 + 4) * 5").parse_expression()->print(), "((3+4)*5)")
    {
        Parser tmp = Parser("3 + 4 * 5; 6 + 7 * 8");
        ASSERT_STR_EQ(tmp.parse_expression()->print(), "(3+(4*5))")
        tmp.expect(SEMICOLON);
        ASSERT_STR_EQ(tmp.parse_expression()->print(), "(6+(7*8))")
    }

    puts(" done");

    printf("testing statement parsing...");

    IfStatement * ifStatement = (IfStatement *) Parser("if 4 + 4 {}").parse_statement();
    ASSERT_EQ(ifStatement->type, IF_STMT)
    ASSERT_STR_EQ(ifStatement->condition->print(), "(4+4)")

    puts(" done");
    END_TEST;
}

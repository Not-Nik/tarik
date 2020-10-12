// Seno (c) Nikolas Wipper 2020

#include "Parser.h"
#include "cli/arguments.h"

#define BEGIN_TEST bool _st = true
#define MID_TEST if (!_st) return _st
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
    MID_TEST;

    printf("testing expression parsing...");

    std::string test = "hey g";

    ASSERT_STR_EQ(Parser("3 + 4 * 5").parse_expression()->print(), "(3+(4*5))")
    ASSERT_STR_EQ(Parser("-3 + -4 * 5").parse_expression()->print(), "(-3+(-4*5))")
    ASSERT_STR_EQ(Parser("-name + 4 * -5").parse_expression()->print(), "(-name+(4*-5))")

    ASSERT_EQ(Parser("if (4 + 4)").parse_statement()->type, IF_STMT)

    puts(" done");
    END_TEST;
}

#define TEST_OPTION 1

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

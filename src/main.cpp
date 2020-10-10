#include "Lexer.h"
#include "cli/arguments.h"

#define BEGIN_TEST bool _st = true
#define MID_TEST if (!_st) return _st
#define END_TEST return _st

#define ASSERT_TOK(type, tok) _st = _st && lexer.peek().id == type; \
_st = _st && lexer.peek().raw == tok;\
if (!_st) { printf("\nFailed for token '%s': expected '%s'", lexer.peek().raw.c_str(), tok); return false; }\
lexer.consume()

bool test() {
    BEGIN_TEST;

    printf("testing lexer...");
    Lexer lexer("hello test4 ( ) +-===- > fn i32 42 12.34 \"a string\"# comment should be ignored\nback");

    ASSERT_TOK(NAME, "hello");
    ASSERT_TOK(NAME, "test4");
    ASSERT_TOK(PAREN_OPEN, "(");
    ASSERT_TOK(PAREN_CLOSE, ")");
    ASSERT_TOK(PLUS, "+");
    ASSERT_TOK(MINUS, "-");
    ASSERT_TOK(DOUBLE_EQUAL, "==");
    ASSERT_TOK(EQUAL, "=");
    ASSERT_TOK(MINUS, "-");
    ASSERT_TOK(GREATER, ">");
    ASSERT_TOK(FUNC, "fn");
    ASSERT_TOK(INT_32, "i32");
    ASSERT_TOK(INTEGER, "42");
    ASSERT_TOK(REAL, "12.34");
    ASSERT_TOK(STRING, "\"a string\"");
    ASSERT_TOK(NAME, "back");

    puts(" done");
    MID_TEST;
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

// tarik (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_TESTING_H_
#define TARIK_SRC_TESTING_H_

static int allocs = 0;

void *operator new(size_t size) {
    void *p = malloc(size);
    allocs++;
    return p;
}

void operator delete(void *p) noexcept {
    free(p);
    allocs--;
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

#define ASSERT_EQ(l, r) { auto save = l; \
_st = l == r;\
count_tested++;\
if (!_st) { printf("\nFailed for expression '%s': expected '%s' found '%s'", #l, #r, std::to_string(save).c_str()); break; }\
count_suc++;}

#define ASSERT_TRUE(e) {_st = e; \
count_tested++;\
if (!_st) { printf("\nFailed for expression: expected '%s' to be true", #e); return false; }\
count_suc++;}

#endif //TARIK_SRC_TESTING_H_

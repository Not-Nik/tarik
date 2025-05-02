// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_TESTING_TESTING_H_
#define TARIK_SRC_TESTING_TESTING_H_

#include <string>
#include <cstdlib>

static int allocs = 0;

bool test();

#define BEGIN_TEST bool _st = true; int count_suc = 0; int count_tested = 0;
#define FIRST_TEST(name) for (int i = 0; i < 1; i++) { printf("Testing %s...", #name);
#define MID_TEST(name) } if (count_tested > 0) printf(" Done! (%i/%i succeeded)\n", count_suc, count_tested); \
count_suc = 0; count_tested = 0;\
for (int i = 0; i < 1; i++) { printf("Testing %s...", #name);
#define END_TEST } if (count_tested > 0) printf(" Done! (%i/%i succeeded)\n", count_suc, count_tested); return _st;

#define ASSERT_TOK(type, tok) {_st = lexer.peek().id == (type);\
bool local_st = _st && lexer.peek().raw == (tok);\
count_tested++;\
if (!local_st) { printf("\nFailed for token '%s': expected '%s'.", lexer.peek().raw.c_str(), tok); }\
else count_suc++;\
lexer.consume();\
_st = _st && local_st;}

#define ASSERT_STR_EQ(l, r) {std::string save = l;\
bool local_st = save == (r);\
count_tested++;\
if (!local_st) { printf("\nFailed for string '%s': expected '%s'.", save.c_str(), r); }\
else count_suc++;\
_st = _st && local_st;}

#define ASSERT_EQ(l, r) { auto save = l; \
bool local_st = (l) == (r);\
count_tested++;\
if (!local_st) { printf("\nFailed for expression '%s': expected '%s' found '%s'.", #l, #r, std::to_string(save).c_str()); }\
else count_suc++;\
_st = _st && local_st;}

#define ASSERT_TRUE(e) {bool local_st = e; \
count_tested++;\
if (!local_st) { printf("\nFailed for expression: expected '%s' to be true.", #e); }\
else count_suc++;\
_st = _st && local_st;}

#define ASSERT_NO_ERROR(b) {bool local_st = b.get_error_count() == 0;\
count_tested++;\
if (!local_st) { printf("\nFailed to parse expression.\n");\
b.print_errors();}\
else count_suc++;\
_st = _st && local_st;}

#endif //TARIK_SRC_TESTING_TESTING_H_

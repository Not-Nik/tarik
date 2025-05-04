// tarik (c) Nikolas Wipper 2020-2025

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_TESTING_TESTING_H_
#define TARIK_SRC_TESTING_TESTING_H_

#include <cstdlib>
#include <print>
#include <string>

#include "error/Bucket.h"
#include "lexical/Lexer.h"
#include "lexical/Token.h"

static int allocs = 0;

class Tester {
    bool st = true;
    int count_suc = 0;
    int count_tested = 0;

public:
    void StartSegment(const std::string &name);
    void EndSegment() const;

    bool Assert(bool condition, std::string msg);

    template <class T, class U>
    void AssertEq(T t, U u);

    void AssertTok(Lexer lexer, TokenType type, const std::string &tok);
    void AssertTrue(bool condition);
    void AssertNoError(Bucket &bucket);
};

template <class T, class U>
void Tester::AssertEq(T t, U u) {
    if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
        Assert(t == u, std::format("\nFailed for expression '{}': found '{}'.", (int) u, (int) t));
    } else {
        Assert(t == u, std::format("\nFailed for expression '{}': found '{}'.", u, t));
    }
}

void selftest(Tester &tester);

#endif //TARIK_SRC_TESTING_TESTING_H_

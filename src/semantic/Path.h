// tarik (c) Nikolas Wipper 2023-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_PATH_H_
#define TARIK_SRC_SEMANTIC_PATH_H_

#include <vector>
#include <string>
#include "lexical/Token.h"

namespace ast
{
class Expression;
}

std::vector<std::string> flatten_path(ast::Expression *path);

class Path {
    std::vector<std::string> parts;
    bool global = false;

public:
    LexerRange origin;

    explicit Path(std::vector<std::string> parts, LexerRange origin);

    static Path from_expression(ast::Expression *path);

    [[nodiscard]] std::string str() const;
    [[nodiscard]] std::vector<std::string> get_parts() const;
    [[nodiscard]] bool is_global() const;
    [[nodiscard]] bool contains_pointer() const;

    Path create_member(const std::string &name) const;
    Path create_member(Token token) const;

    Path get_parent() const;
    Path with_prefix(Path prefix) const;

    bool operator==(const Path &) const;
    bool operator!=(const Path &) const;
};

template <>
struct std::hash<Path> {
    std::size_t operator()(const Path &k) const {
        return std::hash<std::string>()(k.str());
    }
};

#endif //TARIK_SRC_SEMANTIC_PATH_H_

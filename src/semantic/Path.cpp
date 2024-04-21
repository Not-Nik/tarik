// tarik (c) Nikolas Wipper 2023-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Path.h"

std::vector<std::string> flatten_path(ast::Expression *path) {
    if (path->expression_type == ast::NAME_EXPR) {
        return {((ast::NameExpression *) path)->name};
    } else if (path->expression_type == ast::PATH_EXPR) {
        auto *concat = (ast::BinaryOperatorExpression *) path;
        std::vector<std::string> part1 = flatten_path(concat->left);
        std::vector<std::string> part2 = flatten_path(concat->right);

        part1.insert(part1.end(), part2.begin(), part2.end());

        return part1;
    } else if (auto *gl = (ast::PrefixOperatorExpression *) path;
        path->expression_type == ast::PREFIX_EXPR && gl->prefix_type == ast::GLOBAL) {
        std::vector<std::string> path = flatten_path(gl->operand);
        path.insert(path.begin(), "");

        return path;
    }

    return {};
}

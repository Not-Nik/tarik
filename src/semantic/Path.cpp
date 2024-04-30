// tarik (c) Nikolas Wipper 2023-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Path.h"

#include "syntactic/ast/Expression.h"

std::vector<std::string> flatten_path(ast::Expression *path) {
    if (path->expression_type == ast::NAME_EXPR) {
        return {((ast::NameExpression *) path)->name};
    } else if (path->expression_type == ast::PATH_EXPR) {
        auto *concat = (ast::BinaryExpression *) path;
        std::vector<std::string> part1 = flatten_path(concat->left);
        std::vector<std::string> part2 = flatten_path(concat->right);

        part1.insert(part1.end(), part2.begin(), part2.end());

        return part1;
    } else if (auto *gl = (ast::PrefixExpression *) path;
        path->expression_type == ast::PREFIX_EXPR && gl->prefix_type == ast::GLOBAL) {
        std::vector<std::string> path = flatten_path(gl->operand);
        path.insert(path.begin(), "");

        return path;
    }

    return {};
}

Path::Path(std::vector<std::string> parts, LexerRange origin) : parts(parts), origin(origin) {
    if (!this->parts.empty() && this->parts.front().empty()) {
        this->parts.erase(this->parts.begin());
        global = true;
    }
}

Path Path::from_expression(ast::Expression *path) {
    return Path(flatten_path(path), path->origin);
}

std::string Path::str() const {
    std::string res;
    for (auto it = parts.begin(); it != parts.end();) {
        if (it->empty()) {
            ++it;
            continue;
        }
        res += *it;
        if (++it != parts.end()) {
            res += "::";
        }
    }
    return res;
}

std::vector<std::string> Path::get_parts() const {
    return parts;
}

bool Path::is_global() const {
    return global;
}

bool Path::contains_pointer() const {
    return std::find(parts.begin(), parts.end(), "*") != parts.end();
}

Path Path::create_member(std::string name) const {
    std::vector member_parts = parts;
    member_parts.push_back(name);
    return Path(member_parts);
}

Path Path::get_parent() const {
    std::vector parent_parts = parts;
    parent_parts.pop_back();
    return Path(parent_parts);
}

Path Path::with_prefix(Path prefix) const {
    std::vector prefixed_parts = parts;
    prefixed_parts.insert(prefixed_parts.begin(), prefix.parts.begin(), prefix.parts.end());
    return Path(prefixed_parts);
}

bool Path::operator==(const Path &other) const {
    return parts == other.parts;
}

bool Path::operator!=(const Path &other) const {
    return parts != other.parts;
}


// Seno (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_

#include <string>
#include <vector>
#include <variant>
#include <cstdint>

#include "lexical/Token.h"

class StructStatement;

enum TypeSize : std::uint8_t {
    VOID,
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    BOOL
};

inline TypeSize to_typesize(const std::string &s) {
    if (s == "I8")
        return I8;
    if (s == "I16")
        return I16;
    if (s == "I32")
        return I32;
    if (s == "I64")
        return I64;
    if (s == "U8")
        return U8;
    if (s == "U16")
        return U16;
    if (s == "U32")
        return U32;
    if (s == "U64")
        return U64;
    if (s == "F32")
        return F32;
    if (s == "F64")
        return F64;
    if (s == "BOOL")
        return BOOL;
    return (TypeSize) -1;
}

inline std::string to_string(const TypeSize &ts) {
    if (ts == I8)
        return "i8";
    if (ts == I16)
        return "i16";
    if (ts == I32)
        return "i32";
    if (ts == I64)
        return "i64";
    if (ts == U8)
        return "u8";
    if (ts == U16)
        return "u16";
    if (ts == U32)
        return "u32";
    if (ts == U64)
        return "u64";
    if (ts == F32)
        return "f32";
    if (ts == F64)
        return "f64";
    if (ts == BOOL)
        return "bool";
    if (ts == VOID)
        return "void";
    return "";
}

class Type {
    std::variant<TypeSize, std::vector<std::string>> type;

public:
    int pointer_level;
    LexerRange origin;

    Type()
        : type(VOID) {
        pointer_level = 0;
    };

    explicit Type(TypeSize ts, int pl = 0)
        : type(ts),
          pointer_level(pl) {
    }

    explicit Type(std::vector<std::string> t, int pl = 0)
        : type(std::move(t)),
          pointer_level(pl) {
    }

    [[nodiscard]] TypeSize get_primitive() const {
        return std::get<TypeSize>(type);
    }

    [[nodiscard]] std::vector<std::string> get_user() const {
        return std::get<std::vector<std::string>>(type);
    }

    [[nodiscard]] bool is_primitive() const {
        return type.index() == 0;
    }

    [[nodiscard]] bool is_compatible(const Type &t) const {
        if (operator==(Type(VOID)) || t == Type(VOID))
            return false;
        if ((pointer_level == 0 && t.pointer_level > 0) || (t.pointer_level == 0 && pointer_level > 0))
            return false;
        if (pointer_level > 0 && t.pointer_level > 0)
            return true;
        if (type.index() != t.type.index())
            return false;
        if (is_primitive())
            return true;
        return type == t.type;
    }

    [[nodiscard]] bool is_signed_int() const {
        return is_primitive() && (std::get<TypeSize>(type) >= I8 && std::get<TypeSize>(type) <= I64);
    }

    [[nodiscard]] bool is_unsigned_int() const {
        return is_primitive() && (std::get<TypeSize>(type) >= U8 && std::get<TypeSize>(type) <= U64);
    }

    [[nodiscard]] size_t get_integer_bitwidth() const {
        if (!is_primitive())
            return 0;

        auto size = std::get<TypeSize>(type);

        if (size == F32 || size == F64 || size == VOID)
            return 0;
        if (size == I8 || size == U8)
            return 8;
        if (size == I16 || size == U16)
            return 16;
        if (size == I32 || size == U32)
            return 32;
        if (size == I64 || size == U64)
            return 64;
        if (size == BOOL)
            return 1;
        return 0;
    }

    bool operator==(const Type &other) const {
        return type.index() == other.type.index() && pointer_level == other.pointer_level && type == other.type;
    }

    bool operator!=(const Type &other) const {
        return type.index() != other.type.index() || pointer_level != other.pointer_level || type != other.type;
    }

    [[nodiscard]] std::string str() const;
    [[nodiscard]] std::string base() const;
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_

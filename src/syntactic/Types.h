// Seno (c) Nikolas Wipper 2020-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_

#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <format>

#include "lexical/Token.h"
#include "semantic/Path.h"

enum TypeSize : std::uint8_t {
    VOID,
    I8,
    I16,
    I32,
    I64,
    U0,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    BOOL,
    STR
};

inline TypeSize to_typesize(const std::string &s) {
    if (s == "i8")
        return I8;
    if (s == "i16")
        return I16;
    if (s == "i32")
        return I32;
    if (s == "i64")
        return I64;
    if (s == "u0")
        return U0;
    if (s == "u8")
        return U8;
    if (s == "u16")
        return U16;
    if (s == "u32")
        return U32;
    if (s == "u64")
        return U64;
    if (s == "f32")
        return F32;
    if (s == "f64")
        return F64;
    if (s == "bool")
        return BOOL;
    if (s == "str")
        return STR;
    if (s == "void")
        return VOID;
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
    if (ts == U0)
        return "{IntExpression}";
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
    if (ts == STR)
        return "str";
    if (ts == VOID)
        return "void";
    return "";
}

class Type {
    template <class T, class C>
    friend struct std::formatter;

    std::variant<TypeSize, Path> type;

public:
    int pointer_level;
    LexerRange origin;

    Type()
        : type(VOID) {
        pointer_level = 0;
    }

    explicit Type(TypeSize ts, int pl = 0)
        : type(ts),
          pointer_level(pl) {}

    explicit Type(Path p, int pl = 0)
        : type(std::move(p)),
          pointer_level(pl),
          origin(p.origin) {}

    [[nodiscard]] Type get_pointer_to() const {
        Type t;
        t.type = type;
        t.pointer_level = pointer_level + 1;
        t.origin = origin;
        return t;
    }

    [[nodiscard]] Type get_deref() const {
        Type t;
        t.type = type;
        t.pointer_level = pointer_level - 1;
        t.origin = origin;
        return t;
    }

    [[nodiscard]] TypeSize get_primitive() const {
        return std::get<TypeSize>(type);
    }

    [[nodiscard]] Path &get_user() {
        return std::get<Path>(type);
    }

    [[nodiscard]] const Path &get_user() const {
        return std::get<Path>(type);
    }

    [[nodiscard]] Path get_path() const {
        Path path = Path({});
        if (is_primitive()) {
            path = Path({to_string(get_primitive())});
        } else {
            path = get_user();
        }

        for (int i = 0; i < pointer_level; i++)
            path = path.create_member("*");

        return path;
    }

    void set_user(Path user) {
        type = user;
    }

    [[nodiscard]] Type get_result(const Type &t) const {
        if (pointer_level > 0 || !is_primitive())
            return *this;

        if (!is_float() && !is_bool()) {
            TypeSize unsigned_type = VOID, signed_type = VOID;
            if (is_unsigned_int() && t.is_signed_int()) {
                unsigned_type = get_primitive();
                signed_type = t.get_primitive();
            } else if (is_signed_int() && t.is_unsigned_int()) {
                unsigned_type = t.get_primitive();
                signed_type = get_primitive();
            }

            if (unsigned_type == U0)
                signed_type = std::max(signed_type, I8);
            else if (unsigned_type == U8)
                signed_type = std::max(signed_type, I16);
            else if (unsigned_type == U16)
                signed_type = std::max(signed_type, I32);
            else if (unsigned_type == U32)
                signed_type = std::max(signed_type, I64);

            if (unsigned_type != VOID)
                return Type(signed_type);
        }

        return Type(std::max(get_primitive(), t.get_primitive()));
    }

    [[nodiscard]] bool is_compatible(const Type &t) const {
        if (operator==(Type(VOID)) || t == Type(VOID))
            return false;

        // Early bail if the types are the same
        if (*this == t)
            return true;

        // Pointer arithmetic can never be implicit
        if (pointer_level > 0 || t.pointer_level > 0)
            return false;

        if (is_bool() || t.is_bool())
            return false;

        if (is_float() != t.is_float())
            return false;

        if (is_unsigned_int()) {
            // either the other type is also unsigned, or we can cast
            // this one to a signed type without losing information
            return t.is_unsigned_int() || get_integer_bitwidth() < t.get_integer_bitwidth();
        }
        return !t.is_unsigned_int() || t.get_integer_bitwidth() < get_integer_bitwidth();
    }

    [[nodiscard]] bool is_comparable(const Type &t) const {
        // Comparing with a void always fails
        if (operator==(Type(VOID)) || t == Type(VOID))
            return false;

        // Early bail if the types are the same
        if (*this == t)
            return true;

        // Two pointers can always be compared, regardless of level
        if (pointer_level > 0 && t.pointer_level > 0)
            return true;

        if (pointer_level != t.pointer_level) {
            // Pointers are treated as unsigned ints in comparisons
            return (pointer_level == 0 && !is_unsigned_int()) || (t.pointer_level == 0 && !is_unsigned_int());
        }
        // At this point both types aren't pointers

        // User types can't be compared
        if (!is_primitive() || !t.is_primitive())
            return false;

        if (is_float() != t.is_float())
            return false;

        if (is_bool() != t.is_bool())
            return false;

        if (is_unsigned_int()) {
            // either the other type is also unsigned, or we can cast
            // this one to a signed type without losing information
            return t.is_unsigned_int() || get_integer_bitwidth() < t.get_integer_bitwidth();
        }
        return !t.is_unsigned_int() || t.get_integer_bitwidth() < get_integer_bitwidth();
    }

    // does t fit into this
    [[nodiscard]] bool is_assignable_from(const Type &t) const {
        // Assigning to and from a void always fails
        if (operator==(Type(VOID)) || t == Type(VOID))
            return false;

        // Early bail if the types are the same
        if (*this == t)
            return true;

        // Early bail if the pointer levels aren't the same
        if (pointer_level != t.pointer_level)
            return false;

        // Pointers only stay valid if they're the same type.
        // F32 and F64 can't be cast 'trivially' and even
        // integer types can't, if we target a big endian
        // system
        if (pointer_level > 0)
            return type == t.type;

        // Early bail if we're assigning primitive -> user/user -> primitve
        if (type.index() != t.type.index())
            return false;

        if (is_primitive()) {
            // Casting from or to a float should always be explicit
            if (is_float() != t.is_float())
                return false;

            // Casting from a signed int could fail if cast into an unsigned int, no matter the uint's size
            if (is_unsigned_int() && t.is_signed_int())
                return false;
                // Casting from an unsigned int could fail if the signed type isn't bigger
            else if (is_signed_int() && t.is_unsigned_int())
                return get_integer_bitwidth() > t.get_integer_bitwidth();
                // Casting without changing signedness only requires the at least same bitwidth
            else
                return get_integer_bitwidth() >= t.get_integer_bitwidth();
        } else {
            // User types always have to be the same to be compatible
            return type == t.type;
        }
    }

    [[nodiscard]] bool is_primitive() const {
        return type.index() == 0;
    }

    [[nodiscard]] bool is_bool() const {
        return is_primitive() && std::get<TypeSize>(type) == BOOL;
    }

    [[nodiscard]] bool is_float() const {
        return is_primitive() && (std::get<TypeSize>(type) == F32 || std::get<TypeSize>(type) == F64);
    }

    [[nodiscard]] bool is_signed_int() const {
        return is_primitive() && (std::get<TypeSize>(type) >= I8 && std::get<TypeSize>(type) <= I64);
    }

    [[nodiscard]] bool is_unsigned_int() const {
        return is_primitive() && (std::get<TypeSize>(type) >= U0 && std::get<TypeSize>(type) <= U64);
    }

    [[nodiscard]] bool is_copyable() const {
        return is_primitive() || pointer_level > 0;
    }

    [[nodiscard]] size_t get_integer_bitwidth() const {
        if (!is_primitive())
            return 0;

        TypeSize size = std::get<TypeSize>(type);

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
    [[nodiscard]] std::string func_name() const;
};

template <>
struct std::formatter<Type> : std::formatter<std::string> {
    auto format(Type p, std::format_context &ctx) const {
        if (auto *v = std::get_if<TypeSize>(&p.type)) {
            return formatter<string>::format(
                std::format("Type({})", to_string(*v)),
                ctx);
        } else if (auto *v = std::get_if<Path>(&p.type)) {
            return formatter<string>::format(
                std::format("Type({})", v->str()),
                ctx);
        }
    }
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_

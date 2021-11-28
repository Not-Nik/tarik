// Seno (c) Nikolas Wipper 2020

#ifndef TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_
#define TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_

#include <string>

class StructStatement;

enum TypeSize : std::uint8_t {
    VOID, I8, I16, I32, I64, U8, U16, U32, U64, F32, F64, BOOL
};

inline TypeSize to_typesize(const std::string &s) {
    if (s == "I8") return I8;
    if (s == "I16") return I16;
    if (s == "I32") return I32;
    if (s == "I64") return I64;
    if (s == "U8") return U8;
    if (s == "U16") return U16;
    if (s == "U32") return U32;
    if (s == "U64") return U64;
    if (s == "F32") return F32;
    if (s == "F64") return F64;
    if (s == "BOOL") return BOOL;
    return (TypeSize) -1;
}

inline std::string to_string(const TypeSize &ts) {
    if (ts == I8) return "i8";
    if (ts == I16) return "i16";
    if (ts == I32) return "i32";
    if (ts == I64) return "i64";
    if (ts == U8) return "u8";
    if (ts == U16) return "u16";
    if (ts == U32) return "u32";
    if (ts == U64) return "u64";
    if (ts == F32) return "f32";
    if (ts == F64) return "f64";
    if (ts == BOOL) return "bool";
    if (ts == VOID) return "";
    return "";
}

union TypeUnion {
    TypeSize size;
    StructStatement *user_type;
};

class Type {
public:
    bool is_primitive;
    int pointer_level;
    TypeUnion type;

    Type()
        : type(TypeUnion{VOID}) {
        is_primitive = true;
        pointer_level = 0;
    };

    explicit Type(TypeSize ts, int pl = 0)
        : is_primitive(true), pointer_level(pl), type(TypeUnion{ts}) {}

    Type(TypeUnion t, bool prim, int p)
        : is_primitive(prim), pointer_level(p), type(t) {
    }

    [[nodiscard]] bool is_compatible(const Type &t) const {
        if (operator==(Type(VOID)) || t == Type(VOID)) return false;
        if ((!is_integer() && pointer_level == 0 && t.pointer_level > 0) || (!t.is_integer() && t.pointer_level == 0 && pointer_level > 0))
            return false;
        if (is_primitive != t.is_primitive) return false;
        if (is_primitive) return true;
        if (pointer_level > 0 && t.pointer_level > 0) return true;
        return type.user_type == t.type.user_type;
    }

    [[nodiscard]] bool is_signed_int() const {
        return is_primitive && (type.size >= I8 && type.size <= I64);
    }

    [[nodiscard]] bool is_unsigned_int() const {
        return is_primitive && (type.size >= U8 && type.size <= U64);
    }

    [[nodiscard]] bool is_integer() const {
        return is_primitive && (type.size >= I8 && type.size <= U64);
    }

    [[nodiscard]] size_t get_integer_bitwidth() const {
        if (!is_primitive || type.size == F32 || type.size == F64 || type.size == VOID) return 0;
        if (type.size == I8 || type.size == U8) return 8;
        if (type.size == I16 || type.size == U16) return 16;
        if (type.size == I32 || type.size == U32) return 32;
        if (type.size == I64 || type.size == U64) return 64;
        if (type.size == BOOL) return 1;
        return 0;
    }

    bool operator==(const Type &other) const {
        return is_primitive == other.is_primitive && pointer_level == other.pointer_level
            && (is_primitive ? type.size == other.type.size : type.user_type == other.type.user_type);
    }

    bool operator!=(const Type &other) const {
        return is_primitive != other.is_primitive || pointer_level != other.pointer_level
            || (is_primitive ? type.size != other.type.size : type.user_type != other.type.user_type);
    }

    [[nodiscard]] std::string str() const;
};

#endif //TARIK_SRC_SYNTACTIC_EXPRESSIONS_TYPES_H_

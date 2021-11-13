// Seno (c) Nikolas Wipper 2020

#ifndef TARIK_TYPES_H_
#define TARIK_TYPES_H_

#include <string>

class StructStatement;

enum TypeSize : std::uint8_t {
    I8, I16, I32, I64, U8, U16, U32, U64, F32, F64, VOID
};

inline bool is_signed_int(TypeSize ts) {
    if (ts >= I8 && ts <= I64) return true;
    return false;
}

inline bool is_unsigned_int(TypeSize ts) {
    if (ts >= U8 && ts <= U64) return true;
    return false;
}

inline bool is_int(TypeSize ts) {
    if (ts >= I8 && ts <= U64) return true;
    return false;
}

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
        : type(TypeUnion{U8}) {
        is_primitive = true;
        pointer_level = 0;
    };

    explicit Type(TypeSize ts, int pl = 0)
        : is_primitive(true), pointer_level(pl), type(TypeUnion{ts}) {}

    Type(TypeUnion t, bool prim, int p)
        : is_primitive(prim), pointer_level(p), type(t) {
    }

    [[nodiscard]] bool is_compatible(const Type &t) const {
        return (is_primitive && t.is_primitive) || (!is_primitive && !t.is_primitive && type.user_type == t.type.user_type);
    }

    [[nodiscard]] bool is_signed_int() const {
        return is_primitive && ::is_signed_int(type.size);
    }

    [[nodiscard]] bool is_unsigned_int() const {
        return is_primitive && ::is_unsigned_int(type.size);
    }

    bool operator==(const Type &other) const {
        return is_primitive == other.is_primitive && pointer_level == other.pointer_level
            && (is_primitive ? type.size == other.type.size : type.user_type == other.type.user_type);
    }

    bool operator!=(const Type &other) const {
        return is_primitive != other.is_primitive || pointer_level != other.pointer_level
            || (is_primitive ? type.size != other.type.size : type.user_type != other.type.user_type);
    }

    explicit operator std::string() const;
};

#endif //TARIK_TYPES_H_

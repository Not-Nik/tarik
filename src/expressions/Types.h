// Seno (c) Nikolas Wipper 2020

#ifndef TARIK_TYPES_H_
#define TARIK_TYPES_H_

#include <string>

class StructStatement;

enum TypeSize : std::uint8_t {
    I8, I16, I32, I64, U8, U16, U32, U64, F32, F64
};

inline TypeSize to_typesize(const std::string &s) {
    if (s == "I8") return I8;
    if (s == "I16") return I16;
    if (s == "I32") return I32;
    if (s == "I64") return I64;
    if (s == "U8") return U8;
    if (s == "I16") return U16;
    if (s == "I32") return U32;
    if (s == "I64") return U64;
    if (s == "F32") return F32;
    if (s == "F64") return F64;
    return (TypeSize) -1;
}

inline std::string to_string(const TypeSize &ts) {
    if (ts == I8) return "I8";
    if (ts == I16) return "I16";
    if (ts == I32) return "I32";
    if (ts == I64) return "I64";
    if (ts == U8) return "U8";
    if (ts == U16) return "I16";
    if (ts == U32) return "I32";
    if (ts == U64) return "I64";
    if (ts == F32) return "F32";
    if (ts == F64) return "F64";
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
        : type({.size = U8}) {
        is_primitive = true;
        pointer_level = 0;
    };

    explicit Type(TypeSize ts, int pl = 0)
        : type({.size = ts}), is_primitive(true), pointer_level(pl) {}

    Type(TypeUnion t, bool prim, int p)
        : is_primitive(prim), pointer_level(p), type(t) {
    }

    [[nodiscard]] bool is_compatible(const Type &t) const {
        return (is_primitive && t.is_primitive) || (!is_primitive && !t.is_primitive && type.user_type == t.type.user_type);
    }

    bool operator==(const Type &other) const {
        return is_primitive == other.is_primitive && pointer_level == other.pointer_level
            && (is_primitive ? type.size == other.type.size : type.user_type == other.type.user_type);
    }

    explicit operator std::string() const;
};

#endif //TARIK_TYPES_H_

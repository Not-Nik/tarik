// Seno (c) Nikolas Wipper 2020

#ifndef SENO_TYPES_H
#define SENO_TYPES_H

class StructStatement;

enum TypeSize : char {
    I8, I16, I32, I64, U8, U16, U32, U64, F32, F64
};

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
        return (is_primitive && t.is_primitive)
            || (!is_primitive && !t.is_primitive && type.user_type == t.type.user_type);
    }

    bool operator==(const Type &other) {
        return is_primitive == other.is_primitive && pointer_level == other.pointer_level
            && (is_primitive ? type.size == other.type.size : type.user_type == other.type.user_type);
    }
};

#endif //SENO_TYPES_H

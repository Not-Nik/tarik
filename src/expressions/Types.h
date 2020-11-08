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

    Type(TypeUnion t, bool prim, int p)
        : is_primitive(prim), pointer_level(p), type(t) {
    }

    [[nodiscard]] bool is_compatible(const Type &t) const {
        return (is_primitive && t.is_primitive)
            || (!is_primitive && !t.is_primitive && type.user_type == t.type.user_type);
    }
};

#endif //SENO_TYPES_H

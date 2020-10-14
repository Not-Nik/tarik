// Seno (c) Nikolas Wipper 2020

#ifndef SENO_TYPES_H
#define SENO_TYPES_H

class StructStatement;

enum TypeSize : char {
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT32,
    FLOAT64
};

union TypeUnion {
    TypeSize size;
    StructStatement * user_type;
};

class Type {
public:
    bool is_primitive;
    int pointer_level;
    TypeUnion type;

    Type() : type({.size = UINT8}) {
        is_primitive = true;
        pointer_level = 0;
    };

    Type(TypeUnion t, bool prim, int p) :
            is_primitive(prim),
            pointer_level(p),
            type(t) {
    }
};

#endif //SENO_TYPES_H

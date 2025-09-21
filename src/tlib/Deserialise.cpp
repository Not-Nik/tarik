// tarik (c) Nikolas Wipper 2025

#include "Deserialise.h"

#include "syntactic/ast/Statements.h"

void deserialise(std::istream &is, size_t &size) {
    is.read(reinterpret_cast<char *>(&size), sizeof(size));
}

void deserialise(std::istream &is, bool &bol) {
    is.read(reinterpret_cast<char *>(&bol), sizeof(bol));
}

void deserialise(std::istream &is, std::string &str) {
    size_t size;
    deserialise(is, size);
    str.resize(size);
    is.read(reinterpret_cast<char *>(str.data()), size);
}

void deserialise(std::istream &is, LexerRange &range) {
    std::string filename;
    deserialise(is, filename);
    size_t l, p, length;
    deserialise(is, l);
    deserialise(is, p);
    deserialise(is, length);
    range = LexerRange {{(int) l, (int) p, filename}, (int) length};
}

void deserialise(std::istream &is, Path &path) {
    std::vector<std::string> parts;
    deserialise(is, parts);
    path = Path(parts, {});
}

void deserialise(std::istream &is, Type &type) {
    size_t pointer_level;
    deserialise(is, pointer_level);
    bool is_primitive;
    deserialise(is, is_primitive);
    if (is_primitive) {
        size_t primitive;
        deserialise(is, primitive);
        type = Type((TypeSize) primitive, pointer_level);
    } else {
        Path path = Path({}, {});
        deserialise(is, path);
        type = Type(path, pointer_level);
    }
}

void deserialise(std::istream &is, Token &token) {
    deserialise(is, token.origin);
    deserialise(is, token.raw);
}

void deserialise(std::istream &is, aast::VariableStatement *&var) {
    LexerRange origin;
    deserialise(is, origin);
    Type type;
    deserialise(is, type);
    Token name = Token::name({}, {});
    deserialise(is, name);
    var = new aast::VariableStatement(origin, type, name);
}

void deserialise(std::istream &is, aast::FuncDeclareStatement *&decl) {
    LexerRange origin;
    deserialise(is, origin);
    Path path = Path({}, {});
    deserialise(is, path);
    std::string linker_name;
    deserialise(is, linker_name);
    Type return_type;
    deserialise(is, return_type);
    std::vector<aast::VariableStatement *> arguments;
    deserialise(is, arguments);
    bool var_arg;
    deserialise(is, var_arg);
    decl = new aast::FuncDeclareStatement(origin, path, linker_name, return_type, arguments, var_arg);
}

void deserialise(std::istream &is, aast::StructStatement *&decl) {
    LexerRange origin;
    deserialise(is, origin);
    Path path = Path({}, {});
    deserialise(is, path);
    std::vector<aast::VariableStatement *> members;
    deserialise(is, members);
    decl = new aast::StructStatement(origin, path, members);
}

void deserialise(std::istream &is, aast::Statement *&statement) {
    size_t type;
    deserialise(is, type);
    switch (type) {
    case aast::FUNC_DECL_STMT:
        deserialise(is, (aast::FuncDeclareStatement *&) statement);
        break;
    case aast::STRUCT_STMT:
        deserialise(is, (aast::StructStatement *&) statement);
        break;
    default:
        statement = nullptr;
        break;
    }
}

std::vector<aast::Statement *> deserialise(std::istream &is) {
    std::vector<aast::Statement *> statements;
    deserialise(is, statements);
    return statements;
}

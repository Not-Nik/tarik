// tarik (c) Nikolas Wipper 2025

#ifndef TARIK_SRC_TLIB_DESERIALISE_H
#define TARIK_SRC_TLIB_DESERIALISE_H

#include <vector>

#include "semantic/ast/Statements.h"

void deserialise(std::istream &is, size_t &size);
void deserialise(std::istream &is, bool &bol);
void deserialise(std::istream &is, std::string &str);
void deserialise(std::istream &is, LexerRange &range);
void deserialise(std::istream &is, Path &path);
void deserialise(std::istream &is, Type &type);
void deserialise(std::istream &is, Token &token);
void deserialise(std::istream &is, aast::VariableStatement *&var);
void deserialise(std::istream &is, aast::FuncDeclareStatement *&decl);
void deserialise(std::istream &is, aast::StructStatement *&decl);
void deserialise(std::istream &is, aast::Statement *&statement);

template <class T>
void deserialise(std::istream &is, std::vector<T> &vec) {
    size_t size;
    deserialise(is, size);
    vec.resize(size);
    for (size_t i = 0; i < size; i++) {
        deserialise(is, vec[i]);
    }
}

std::vector<aast::Statement *> deserialise(std::istream &is);

#endif //TARIK_SRC_TLIB_DESERIALISE_H

// tarik (c) Nikolas Wipper 2025

#ifndef TARIK_SRC_TLIB_SERIALISE_H
#define TARIK_SRC_TLIB_SERIALISE_H

#include <sstream>
#include <string>
#include <vector>

#include "semantic/ast/Statements.h"

void serialise(std::ostream& os, size_t siz);
void serialise(std::ostream& os, bool bol);
void serialise(std::ostream& os, const std::string &string);
void serialise(std::ostream &os, const LexerRange &range);
void serialise(std::ostream& os, Path path);
void serialise(std::ostream& os, Type type);
void serialise(std::ostream& os, Token token);
void serialise(std::ostream& os, aast::VariableStatement *decl);
void serialise(std::ostream& os, aast::FuncDeclareStatement *decl);
void serialise(std::ostream& os, aast::StructStatement *decl);
void serialise(std::ostream& os, aast::ImportStatement *imp);
void serialise(std::ostream &os, aast::Statement *decl);

template <class T>
void serialise(std::ostream& os, std::vector<T> vec) {
    serialise(os, vec.size());
    for (auto el : vec) {
        serialise(os, el);
    }
}

#endif //TARIK_SRC_TLIB_SERIALISE_H
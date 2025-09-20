// tarik (c) Nikolas Wipper 2025

#ifndef TARIK_SRC_TLIB_EXPORT_H
#define TARIK_SRC_TLIB_EXPORT_H

#include <vector>
#include <sstream>

#include "semantic/ast/Statements.h"

class Export {
    std::stringstream buffer;

public:
    void generate_statements(const std::vector<aast::Statement *> &s);

    void write_file(const std::string &to);
};


#endif // TARIK_SRC_TLIB_EXPORT_H

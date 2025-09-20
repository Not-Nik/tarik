// tarik (c) Nikolas Wipper 2025

#ifndef TARIK_SRC_TLIB_IMPORT_H
#define TARIK_SRC_TLIB_IMPORT_H

#include "semantic/ast/Statements.h"

std::vector<aast::Statement*> import_statements(std::filesystem::path path);

#endif //TARIK_SRC_TLIB_IMPORT_H
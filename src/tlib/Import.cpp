// tarik (c) Nikolas Wipper 2025

#include "Import.h"

#include <fstream>

#include "Deserialise.h"

std::vector<aast::Statement*> import_statements(std::filesystem::path path) {
    std::ifstream file(path);

    char MAGIC[4];
    file.read(MAGIC, 4);

    if (memcmp(MAGIC, "TLIB", 4) != 0) {
        return {};
    }

    return deserialise(file);
}

// tarik (c) Nikolas Wipper 2025

#include "Export.h"

#include "tlib/Serialise.h"

void Export::generate_statements(const std::vector<aast::Statement *> &s) {
    serialise(buffer, s);
}

void Export::write_file(const std::string &to) {
    std::ofstream file(to, std::ios::binary);

    constexpr char MAGIC[4] = {'T', 'L', 'I', 'B'};

    file.write(MAGIC, 4);

    file << buffer.rdbuf();
}


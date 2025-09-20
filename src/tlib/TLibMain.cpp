// tarik (c) Nikolas Wipper 2025

#include <iostream>

#include "Import.h"
#include "cli/Arguments.h"
#include "Version.h"
#include "semantic/ast/Statements.h"

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "tarik-tlib");

    Option *version = parser.add_option("version", "Miscellaneous", "Display the compiler version");

    for (const auto &option : parser) {
        if (option == version) {
            std::cout << version_id << " tarik compiler tester version " << version_string << "\n";
            return 0;
        }
    }

    if (parser.get_inputs().empty()) {
        std::cerr << "error: No input file\n";
        return 1;
    }

    std::filesystem::path input = parser.get_inputs()[0];

    if (!exists(input)) {
        std::cerr << "error: Input file " << input << " does not exist\n";
        return 1;
    }

    std::vector<aast::Statement*> statements = import_statements(input);

    for (auto *statement : statements) {
        std::println("{}", statement->print());
    }
}

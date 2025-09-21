// tarik (c) Nikolas Wipper 2025

#include <iostream>

#include "Export.h"
#include "Import.h"
#include "cli/Arguments.h"
#include "Version.h"
#include "semantic/ast/Statements.h"

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "tarik-tlib");

    Option *version = parser.add_option("version", "Miscellaneous", "Display the compiler version");
    Option *merge_option = parser.add_option("merge",
                                             "Actions",
                                             "Merge a library into the input library",
                                             true,
                                             "library");

    std::filesystem::path merge;

    for (const auto &option : parser) {
        if (option == version) {
            std::cout << version_id << " tarik compiler tester version " << version_string << "\n";
            return 0;
        } else if (option == merge_option) {
            merge = option.argument;

            if (!exists(merge)) {
                std::cerr << "error: Input file " << merge << " does not exist\n";
                return 1;
            }
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

    std::vector<aast::Statement *> statements = import_statements(input);

    if (!merge.empty()) {
        std::vector<aast::Statement *> merge_statements = import_statements(merge);

        lift_up_undefined(merge_statements, Path({merge.stem()}, {}));

        Export exporter;
        exporter.generate_statements(statements);
        exporter.generate_statements(merge_statements);
        exporter.write_file(input);
    } else {
        for (auto *statement : statements) {
            std::println("{}", statement->print());
        }
    }
}

// tarik (c) Nikolas Wipper 2025

#include <iostream>

#include "cli/Arguments.h"
#include "Testing.h"
#include "Version.h"

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "tarik");

    Option *version = parser.add_option("version", "Miscellaneous", "Display the compiler version");

    if (argc > 1) {
        for (const auto &option : parser) {
            if (option == version) {
                std::cout << version_id << " tarik compiler version " << version_string << "\n";
                return 0;
            }
        }
    }

    test();
}

// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "comperr.h"
#include "Testing.h"
#include "cli/Arguments.h"

#include "codegen/LLVM.h"
#include "syntactic/Parser.h"
#include "semantic/Analyser.h"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

constinit const char *version_id = "Generic";
constinit const char *version_string = "0.0.1a";

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "tarik");

    Option *test_option = parser.add_option("test", "Run internal tarik tests");
    Option *output_option = parser.add_option("output", "Output to file", true, "file", 'o');

    Option *re_emit_option = parser.add_option("re-emit", "Parse code, and re-emit it based on the internal AST");
    Option *emit_llvm_option = parser.add_option("emit-llvm", "Emit generated LLVM IR");
    Option *override_triple = parser.add_option("target",
                                                "Set the target-triple (defaults to '" + LLVM::default_triple + "')",
                                                true,
                                                "triple",
                                                't');
    Option *search_path = parser.add_option("search-path", "Add an import search path", true, "path", 's');
    Option *version = parser.add_option("version", "Display the compiler version");
    Option *list_targets = parser.add_option("list-targets", "List all available targets");

    bool re_emit = false, emit_llvm = false;
    std::string output_filename, triple = LLVM::default_triple;
    std::vector<fs::path> search_paths;

    for (const auto &option : parser) {
        if (option == test_option) {
            int r = test();
            if (r)
                puts("Tests succeeded");
            else
                puts("Tests failed");
            return !r;
        } else if (option == re_emit_option) {
            re_emit = true;
        } else if (option == emit_llvm_option) {
            emit_llvm = true;
        } else if (option == output_option) {
            output_filename = option.argument;
        } else if (option == override_triple) {
            triple = option.argument;
        } else if (option == search_path) {
            search_paths.emplace_back(option.argument);
        } else if (option == version) {
            LLVM::force_init();
            std::cout << version_id << " tarik compiler version " << version_string << "\n";
            std::cout << "Default target: " << LLVM::default_triple << "\n";
            return 0;
        } else if (option == list_targets) {
            LLVM::force_init();
            std::cout << "Available LLVM targets:\n";
            for (const auto &t : LLVM::get_available_triples()) {
                std::cout << "    " << t << "\n";
            }
            return 0;
        }
    }

    if (re_emit && emit_llvm) {
        std::cerr << "Options 're-emit' and 'emit-llvm' are mutually-exclusive; using 'emit-llvm'...\n";
    }

    if (parser.get_inputs().size() > 1) {
        std::cerr << "error: Multiple input files\n";
        return 1;
    }

    if (parser.get_inputs().size() == 0) {
        std::cerr << "error: No input file\n";
        return 1;
    }

    std::string input = parser.get_inputs()[0];
    fs::path input_path = input;

    if (!exists(input_path)) {
        std::cerr << "'" << input_path << "' doesn't exist...\n";
        return 1;
    }

    fs::path output_path = input_path;
    if (output_filename.empty()) {
        std::string new_extension;
        if (re_emit && !emit_llvm) {
            new_extension = ".re.tk";
        } else if (emit_llvm) {
            new_extension = ".ll";
        } else {
            new_extension = ".o";
        }
        output_path.replace_extension(new_extension);
    } else
        output_path = output_filename;

    if (output_path.has_parent_path() && !exists(output_path.parent_path())) {
        create_directories(output_path.parent_path());
    }

    Bucket error_bucket;
    Parser p(input_path, &error_bucket, search_paths);

    std::vector<Statement *> statements;
    do {
        statements.push_back(p.parse_statement());
    } while (statements.back());
    statements.pop_back();

    if (error_bucket.error_count() == 0) {
        Analyser analyser(&error_bucket);
        analyser.verify_statements(statements);
        statements = analyser.finish();
    }

    if (error_bucket.error_count() == 0) {
        std::ofstream out(output_path);
        if (re_emit && !emit_llvm) {
            for (auto s : statements) {
                out << s->print() << "\n\n";
            }
            out.put('\n');
        }
        if (!re_emit) {
            LLVM generator(input);
            if (!triple.empty() && !LLVM::is_valid_triple(triple)) {
                std::cerr << "Invalid triple '" << triple << "'\n";
                return 1;
            }
            generator.generate_statements(statements);
            if (emit_llvm)
                generator.dump_ir(output_path);
            else
                generator.write_object_file(output_path, triple);
        }
    }

    std::for_each(statements.begin(), statements.end(), [](auto &p) { delete p; });
    error_bucket.print_errors();


    return endfile() ? 0 : 1;
}

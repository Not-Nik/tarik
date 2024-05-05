// tarik (c) Nikolas Wipper 2020-2024

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

    // Code Analsis
    Option *search_path = parser.add_option("search-path",
                                            "Code Analysis",
                                            "Add an import search path",
                                            true,
                                            "path",
                                            's');

    // Code Generation
    Option *code_model = parser.add_option("Ccode-model", "Code Generation", "Set the code model", true, "model");
    Option *optimise = parser.add_option("Coptimise",
                                         "Code Generation",
                                         "Set the optimisation level",
                                         true,
                                         "level",
                                         'O');
    Option *pic = parser.add_option("Cpic", "Code Generation", "Enable PIC");
    Option *override_triple = parser.add_option("Ctarget",
                                                "Code Generation",
                                                "Set the target-triple (defaults to '" + LLVM::default_triple + "')",
                                                true,
                                                "triple",
                                                't');

    // Miscellaneous
    Option *list_targets = parser.add_option("list-targets", "Miscellaneous", "List all available targets");
    Option *test_option = parser.add_option("test", "Miscellaneous", "Run internal tarik tests");
    Option *version = parser.add_option("version", "Miscellaneous", "Display the compiler version");

    // Output
    Option *emit_aast_option = parser.add_option("emit-aast",
                                                 "Output",
                                                 "Parse code, analyse, and re-emit it based on the internal AST");
    Option *emit_assembly = parser.add_option("emit-assembly", "Output", "Emit Assembly", false, "", 's');
    Option *emit_ast_option = parser.add_option("emit-ast",
                                                "Output",
                                                "Parse code, and re-emit it based on the internal AST");
    Option *emit_llvm_option = parser.add_option("emit-llvm", "Output", "Emit generated LLVM IR");
    Option *output_option = parser.add_option("output", "Output", "Output to file", true, "file", 'o');

    LLVM::Config config;
    bool emit_aast = false, emit_ast = false, emit_llvm = false;
    std::string output_filename;
    std::vector<fs::path> search_paths;

    for (const auto &option : parser) {
        if (option == search_path) {
            search_paths.emplace_back(option.argument);
        } else if (option == code_model) {
            if (option.argument == "tiny")
                config.code_model = llvm::CodeModel::Tiny;
            else if (option.argument == "small")
                config.code_model = llvm::CodeModel::Small;
            else if (option.argument == "kernel")
                config.code_model = llvm::CodeModel::Kernel;
            else if (option.argument == "medium")
                config.code_model = llvm::CodeModel::Medium;
            else if (option.argument == "large")
                config.code_model = llvm::CodeModel::Large;
            else
                std::cerr << "error: Unknown code model '" << option.argument << "'\n";
        } else if (option == optimise) {
            if (option.argument == "0")
                config.optimisation_level = llvm::CodeGenOptLevel::None;
            else if (option.argument == "1")
                config.optimisation_level = llvm::CodeGenOptLevel::Less;
            else if (option.argument == "2")
                config.optimisation_level = llvm::CodeGenOptLevel::Default;
            else if (option.argument == "3")
                config.optimisation_level = llvm::CodeGenOptLevel::Aggressive;
            else
                std::cerr << "error: Unknown optimisation level '" << option.argument << "'\n";
        } else if (option == pic) {
            config.pic = true;
        } else if (option == override_triple) {
            config.triple = option.argument;
        } else if (option == list_targets) {
            LLVM::force_init();
            std::cout << "Available LLVM targets:\n";
            for (const auto &t : LLVM::get_available_triples()) {
                std::cout << "    " << t << "\n";
            }
            return 0;
        } else if (option == test_option) {
            int r = test();
            if (r)
                std::cout << "Tests succeeded\n";
            else
                std::cerr << "Tests failed\n";
            return !r;
        } else if (option == version) {
            LLVM::force_init();
            std::cout << version_id << " tarik compiler version " << version_string << "\n";
            std::cout << "Default target: " << LLVM::default_triple << "\n";
            return 0;
        } else if (option == emit_aast_option) {
            emit_aast = true;
        } else if (option == emit_assembly) {
            config.output = LLVM::Config::Output::Assembly;
        } else if (option == emit_ast_option) {
            emit_ast = true;
        } else if (option == emit_llvm_option) {
            emit_llvm = true;
        } else if (option == output_option) {
            output_filename = option.argument;
        }
    }

    if (emit_ast && emit_llvm) {
        std::cerr << "error: Options 're-emit' and 'emit-llvm' are mutually-exclusive\n";
        return 1;
    }

    if (parser.get_inputs().size() > 1) {
        std::cerr << "error: Multiple input files\n";
        return 1;
    }

    if (parser.get_inputs().empty()) {
        std::cerr << "error: No input file\n";
        return 1;
    }

    std::string input = parser.get_inputs()[0];
    fs::path input_path = input;

    if (!exists(input_path)) {
        std::cerr << "error: '" << input_path << "' doesn't exist...\n";
        return 1;
    }

    fs::path output_path = input_path;
    if (output_filename.empty()) {
        std::string new_extension;
        if (emit_ast) {
            new_extension = ".re.tk";
        } else if (emit_llvm) {
            new_extension = ".ll";
        } else if (config.output == LLVM::Config::Output::Assembly) {
            new_extension = ".s";
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

    std::vector<ast::Statement *> statements;
    do {
        statements.push_back(p.parse_statement());
    } while (statements.back());
    statements.pop_back();

    int result = 0;
    std::ofstream out(output_path);
    if (emit_ast) {
        for (auto *s : statements) {
            out << s->print() << "\n\n";
        }
        out.put('\n');
    } else {
        std::vector<aast::Statement *> analysed_statements;
        if (error_bucket.get_error_count() == 0) {
            Analyser analyser(&error_bucket);
            analyser.analyse(statements);
            analysed_statements = analyser.finish();
        }

        if (error_bucket.get_error_count() == 0) {
            if (emit_aast) {
                for (auto *s : analysed_statements) {
                    out << s->print() << "\n\n";
                }
                out.put('\n');
            } else {
                LLVM generator(input);
                generator.generate_statements(analysed_statements);
                if (emit_llvm)
                    generator.dump_ir(output_path);
                else
                    result = generator.write_file(output_path, config);
            }
        }

        std::for_each(analysed_statements.begin(), analysed_statements.end(), [](auto &p) { delete p; });
    }

    std::for_each(statements.begin(), statements.end(), [](auto &p) { delete p; });
    error_bucket.print_errors();

    return endfile() ? result : 1;
}

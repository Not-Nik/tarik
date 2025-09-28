// tarik (c) Nikolas Wipper 2020-2025

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "comperr.h"

#include "cli/Arguments.h"
#include "codegen/LLVM.h"
#include "lifetime/Analyser.h"
#include "semantic/Analyser.h"
#include "syntactic/Parser.h"
#include "Version.h"

#include <fstream>
#include <iostream>
#include <filesystem>

#include "System.h"
#include "tlib/Export.h"
#include "tlib/Import.h"

namespace fs = std::filesystem;

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "tarik");

    // Code Analsis
    Option *search_path = parser.add_option("import",
                                            "Code Analysis",
                                            "Import declarations from a .tlib file",
                                            "path",
                                            'I');

    // Code Generation
    Option *code_model = parser.add_option("Ccode-model", "Code Generation", "Set the code model", "model");
    Option *optimise = parser.add_option("Coptimise",
                                         "Code Generation",
                                         "Set the optimisation level",
                                         "level",
                                         'O');
    Option *pic = parser.add_option("Cpic", "Code Generation", "Enable PIC");
    Option *override_triple = parser.add_option("Ctarget",
                                                "Code Generation",
                                                "Set the target-triple (defaults to '" + LLVM::default_triple + "')",
                                                "triple",
                                                't');

    // Miscellaneous
    Option *list_targets = parser.add_option("list-targets", "Miscellaneous", "List all available targets");
    Option *version = parser.add_option("version", "Miscellaneous", "Display the compiler version");

    // Output
    Option *emit_option = parser.add_option("emit",
                                            "Output",
                                            "Add type to the list of emitted output.\n"
                                            " - asm - name.s - Assembly code\n"
                                            " - lib - name.tlib - Library metadata\n"
                                            " - llvm - name.ll - LLVM IR\n"
                                            " - obj - name.o - Object file\n"
                                            " - sem - name.sem.tk - Code based on the semantic AST\n"
                                            " - syn - name.syn.tk - Code based on the syntactic AST",
                                            "aast|ast|asm|llvm|obj");

    Option *output_option = parser.add_option("output",
                                              "Output",
                                              "Set output file stem, extension is ignored",
                                              "file",
                                              'o');

    LLVM::Config config;
    bool emit_aast = false, emit_ast = false, emit_asm = false, emit_llvm = false, emit_obj = false, emit_lib = false;
    std::string output_filename;
    std::unordered_map<std::string, std::vector<aast::Statement *>> libraries;

    for (const auto &option : parser) {
        if (option == search_path) {
            std::filesystem::path input = option.argument;
            std::vector<aast::Statement *> statements = import_statements(input);
            if (libraries.contains(input.stem())) {
                std::cerr << "warning: Duplicate library '" << input.stem() << "' ignored\n";
                continue;
            }
            lift_up_undefined(statements, Path({input.stem()}, {}));
            libraries.emplace(input.stem(), statements);
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
        } else if (option == version) {
            LLVM::force_init();
            std::cout << version_id << " tarik compiler version " << version_string << "\n";
            std::cout << "Default target: " << LLVM::default_triple << "\n";
            return 0;
        } else if (option == emit_option) {
            if (option.argument == "sem")
                emit_aast = true;
            else if (option.argument == "syn")
                emit_ast = true;
            else if (option.argument == "asm")
                emit_asm = true;
            else if (option.argument == "lib")
                emit_lib = true;
            else if (option.argument == "llvm")
                emit_llvm = true;
            else if (option.argument == "obj")
                emit_obj = true;
        } else if (option == output_option) {
            output_filename = option.argument;
        }
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

    fs::path aast_path, ast_path, asm_path, llvm_path, obj_path, lib_path;
    if (output_filename.empty()) {
        aast_path = ast_path = asm_path = llvm_path = obj_path = lib_path = input_path;
    } else {
        aast_path = ast_path = asm_path = llvm_path = obj_path = lib_path = output_filename;
    }

    aast_path.replace_extension(".sem.tk");
    ast_path.replace_extension(".syn.tk");
    asm_path.replace_extension(".s");
    llvm_path.replace_extension(".ll");
    obj_path.replace_extension(".o");
    lib_path.replace_extension(".tlib");

    std::filesystem::path wd = std::filesystem::current_path();
    std::filesystem::current_path(input_path.parent_path());

    Bucket error_bucket;
    Parser p(input_path.filename(), &error_bucket);

    std::vector<ast::Statement *> statements;
    do {
        statements.push_back(p.parse_statement());
    } while (statements.back());
    statements.pop_back();

    std::filesystem::current_path(wd);

    int result = 0;
    if (emit_ast) {
        std::ofstream out(ast_path);
        for (auto *s : statements) {
            out << s->print() << "\n\n";
        }
        out.put('\n');
    }
    std::vector<aast::Statement *> analysed_statements;
    if (error_bucket.get_error_count() == 0) {
        Analyser analyser(&error_bucket, libraries);
        analyser.analyse(statements);
        analysed_statements = analyser.finish();

        if (error_bucket.get_error_count() == 0) {
            lifetime::Analyser lifetime_analyser(&error_bucket, &analyser);
            lifetime_analyser.analyse(analysed_statements);
        }
    }

    if (error_bucket.get_error_count() == 0) {
        if (emit_aast) {
            std::ofstream out(aast_path);
            for (auto *s : analysed_statements) {
                out << s->print() << "\n\n";
            }
            out.put('\n');
        }
        if (emit_lib) {
            Export exporter;
            exporter.generate_statements(analysed_statements);
            exporter.write_file(lib_path);
        }

        if (emit_llvm || emit_asm || emit_obj) {
            LLVM generator(input);
            generator.generate_statements(analysed_statements);
            if (emit_llvm)
                generator.dump_ir(llvm_path);
            if (emit_asm) {
                config.output = LLVM::Config::Output::Assembly;
                result = generator.write_file(asm_path, config);
            }
            if (emit_obj) {
                config.output = LLVM::Config::Output::Object;
                result = generator.write_file(obj_path, config);
            }
        }
    }

    std::for_each(analysed_statements.begin(), analysed_statements.end(), [](auto &p) { delete p; });
    std::for_each(statements.begin(), statements.end(), [](auto &p) { delete p; });
    error_bucket.print_errors();

    return endfile() ? result : 1;
}

// tarik (c) Nikolas Wipper 2020

#include "comperr.h"
#include "Testing.h"
#include "cli/arguments.h"

#include "codegen/LLVM.h"
#include "syntactic/Parser.h"
#include "semantic/Analyser.h"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "tarik");

    Option *test_option = parser.add_option("test", "Run internal tarik tests", false);
    Option *output_option = parser.add_option("output", "Output to file", true, "file", 'o');

    Option *re_emit_option = parser.add_option("re-emit", "Parse code, and re-emit it based on the internal AST", false);
    Option *emit_llvm_option = parser.add_option("emit-llvm", "Emit generated LLVM IR", false);
    Option *override_triple = parser.add_option("target", "Set the target triple (defaults to '" + LLVM::default_triple + "')", true, "triple", 't');

    bool re_emit = false, emit_llvm = false;
    std::string output_filename, triple = LLVM::default_triple;

    for (auto option: parser) {
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
        }
    }

    if (re_emit && emit_llvm) {
        std::cerr << "Options 're-emit' and 'emit-llvm' are mutually-exclusive; using 'emit-llvm'...\n";
    }

    if (!output_filename.empty() && parser.get_inputs().size() > 1) {
        std::cerr << "An explicit output file with multiple input files will overwrite compilation results. (Where'd you want me to write the rest?)";
        return 1;
    }

    for (const auto &input: parser.get_inputs()) {
        fs::path input_path = fs::absolute(input);
        fs::path output_path;
        if (output_filename.empty()) output_path = fs::absolute(input_path).replace_extension(".o");
        else output_path = fs::absolute(output_filename);
        Parser p(input_path);

        std::vector<Statement *> statements;
        do {
            statements.push_back(p.parse_statement());
        } while (statements.back());
        statements.pop_back();

        Analyser analyser;
        if (!analyser.verify_statements(statements)) break;

        endfile();

        if (Parser::error_count() == 0) {
            std::ofstream out(output_filename);
            if (re_emit && !emit_llvm) {
                for (auto s: statements) {
                    out << s->print() << "\n\n";
                }
                out.put('\n');
            }
            if (!re_emit || !emit_llvm) {
                LLVM generator(input);
                if (!triple.empty() && !LLVM::is_valid_triple(triple)) {
                    std::cerr << "Invalid triple '" << triple << "'";
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
    }

    return 0;
}

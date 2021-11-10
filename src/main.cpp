// tarik (c) Nikolas Wipper 2020

#include "comperr.h"
#include "Testing.h"
#include "cli/arguments.h"
#include "syntactic/Parser.h"
#include "semantic/Analyser.h"

#include <fstream>

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "tarik");

    Option *test_option = parser.add_option("test", "Run internal tarik tests", false, "");
    Option *re_emit_option = parser.add_option("re-emit", "Parse code, and re-emit it based on the internal AST", false, "");
    Option *output_option = parser.add_option("output", "Output to file", true, "file", 'o');

    bool re_emit = false;
    std::string output_filename = "trk.out";

    for (auto option : parser) {
        if (option == test_option) {
            int r = test();
            if (r)
                puts("Tests succeeded");
            else
                puts("Tests failed");
            return !r;
        } else if (option == re_emit_option) {
            re_emit = true;
        } else if (option == output_option) {
            output_filename = option.argument;
        }
    }

    for (const auto &input: parser.get_inputs()) {
        Parser p(input);

        std::vector<Statement *> statements;
        do {
            statements.push_back(p.parse_statement());
        } while (statements.back());
        statements.pop_back();

        Analyser analyser;
        if (!analyser.verify_statements(statements)) break;

        endfile();

        if (Parser::error_count() == 0) {
            if (re_emit) {
                std::ofstream out(output_filename);
                for (auto s: statements) {
                    out << s->print() << "\n\n";
                }
                out.put('\n');
            }
        }

        std::for_each(statements.begin(), statements.end(), [](auto &p) {delete p;});
    }

    return 0;
}

// tarik (c) Nikolas Wipper 2025

#include <iostream>
#include <fstream>
#include <set>

#include "cli/Arguments.h"
#include "Testing.h"
#include "Version.h"
#include "codegen/LLVM.h"
#include "lifetime/Analyser.h"
#include "syntactic/Parser.h"

void compile_test_file(Bucket &bucket, const std::string &file_name) {
    Parser p(file_name, &bucket, {});

    std::vector<ast::Statement *> statements;
    do {
        statements.push_back(p.parse_statement());
    } while (statements.back());
    statements.pop_back();

    std::vector<aast::Statement *> analysed_statements;
    if (bucket.get_error_count() != 0)
        return;
    Analyser analyser(&bucket);
    analyser.analyse(statements);
    analysed_statements = analyser.finish();

    if (bucket.get_error_count() != 0)
        return;
    lifetime::Analyser lifetime_analyser(&bucket, &analyser);
    lifetime_analyser.analyse(analysed_statements);

    if (bucket.get_error_count() != 0)
        return;
    LLVM generator(file_name);
    generator.generate_statements(analysed_statements);
}

void read_test_file(Tester &tester, const std::string &file_name) {
    std::vector<std::string> file_lines;
    std::string line;

    std::ifstream file(file_name);

    while (std::getline(file, line)) {
        file_lines.push_back(line);
    }

    bool valid_test = false;
    std::optional<bool> should_pass;

    int line_number = 0;

    std::set<int> expected_errors;
    std::set<int> expected_warnings;

    for (auto line : file_lines) {
        line_number++;

        // ltrim the line
        line.erase(line.begin(),
                   std::find_if(line.begin(),
                                line.end(),
                                [](unsigned char ch) {
                                    return !std::isspace(ch);
                                }));

        if (line.starts_with("# /tk ")) {
            std::string command = line.substr(6);

            if (command == "test") {
                if (valid_test)
                    tester.Assert(false, "Multiple test commands in one file");
                valid_test = true;
            } else if (command == "pass") {
                if (should_pass.has_value())
                    tester.Assert(false, "Multiple pass/fail commands in one file");
                should_pass = true;
            } else if (command == "fail") {
                if (should_pass.has_value())
                    tester.Assert(false, "Multiple pass/fail commands in one file");
                should_pass = false;
            } else if (command.starts_with("error")) {
                expected_errors.emplace(line_number + 1);
            } else if (command.starts_with("warning")) {
                expected_warnings.emplace(line_number + 1);
            }
        }
    }

    Bucket bucket;

    compile_test_file(bucket, file_name);

    if (should_pass.has_value()) {
        if (should_pass.value()) {
            tester.AssertNoError(bucket);
        } else {
            std::vector<Error> errors = bucket.get_errors();

            for (auto &error : errors) {
                if (error.kind == ErrorKind::ERROR && !expected_errors.contains(error.origin.l)) {
                    tester.Assert(false,
                                  std::format("Extra error in {}:{} : {}",
                                              error.origin.l,
                                              error.origin.p,
                                              error.message));
                } else if (error.kind == ErrorKind::WARNING && !expected_warnings.contains(error.origin.l)) {
                    tester.Assert(false,
                                  std::format("Extra warning in {}:{} : {}",
                                              error.origin.l,
                                              error.origin.p,
                                              error.message));
                }
            }

            tester.Assert(true, "");
        }
    }
}

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

    Tester tester;

    selftest(tester);

    tester.StartSegment("passing tests");

    for (auto &file : std::filesystem::directory_iterator("tests/passing")) {
        std::string file_name = file.path().string();
        if (file_name.ends_with(".tk"))
            read_test_file(tester, file_name);
    }

    tester.EndSegment();
    tester.StartSegment("failing tests");

    for (auto &file : std::filesystem::directory_iterator("tests/failing")) {
        std::string file_name = file.path().string();
        if (file_name.ends_with(".tk"))
            read_test_file(tester, file_name);
    }

    tester.EndSegment();
}

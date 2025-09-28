// tarik (c) Nikolas Wipper 2025

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <filesystem>
#include <utility>

#include "cli/Arguments.h"
#include "System.h"
#include "Version.h"

#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>
#include <llvm/Support/Program.h>

static bool volatile_ = false;

struct Paths {
    std::filesystem::path temet;
    std::filesystem::path tarik;
    std::filesystem::path libraries;

    explicit Paths(const char *argv0) {
        temet = get_executable_path(argv0);
        tarik = temet.parent_path() / "tarik";
        if (!exists(tarik))
            tarik = find_executable("tarik");
        if (exists(tarik))
            libraries = tarik.parent_path().parent_path() / "lib";
    }
};

int build(const Paths &paths, std::filesystem::path in, std::filesystem::path out);

std::filesystem::path make_output_path(std::filesystem::path out, std::string name, std::string version) {
    return out / name / version / name;
}

struct Dependency {
    std::string name;
    std::string version;
    bool system;

    Dependency(const std::string &name, const std::string &version, bool system)
        : name(name),
          version(version),
          system(system) {}

    int build(const Paths &paths, std::filesystem::path out) {
        if (system) {
            std::filesystem::path path = paths.libraries / name;
            if (!exists(path)) {
                std::cerr << "error: couldn't find system library '" << name << "'\n";
                return 1;
            }

            int ret = ::build(paths, path, std::move(out));
            if (volatile_) {
                std::cerr << " * Exiting " << path << "\n";
            }
            return ret;
        }
        return 0;
    }
};

int build(const Paths &paths, std::filesystem::path in, std::filesystem::path out) {
    if (volatile_) {
        std::cerr << " * Entering " << in << "\n";
    }

    if (!std::filesystem::exists(in / "Temet.toml")) {
        std::cerr << "error: Temet.toml does not exist\n";
        return 1;
    }

    auto maybe_config = toml::parse_file((in / "Temet.toml").string());

    if (maybe_config.failed()) {
        std::cerr << "error: failed to parse Temet.toml\n";
        return 1;
    }

    auto config = maybe_config.table();

    auto project = config["package"];

    if (!project.is_table()) {
        std::cerr << "error: Temet.toml should contain table 'package'\n";
        return 1;
    }

    if (!project["name"].is_string()) {
        std::cerr << "error: Temet.toml should contain string 'project.name'\n";
        return 1;
    }

    std::string name = project["name"].as_string()->get();
    std::string version = project["version"].value_or("");

    toml::table dependencies;

    if (config.contains("dependencies")) {
        if (config["dependencies"].is_table()) {
            dependencies = *config["dependencies"].as_table();
        } else {
            std::cerr << "error: 'dependencies' should be a table \n";
            return 1;
        }
    } else {
        dependencies = toml::table();
    }

    std::vector<Dependency> deps;

    for (auto &&[key, value] : dependencies) {
        std::string dep_name = {key.begin(), key.end()};

        switch (value.type()) {
        case toml::node_type::table: {
            toml::table table = *value.as_table();
            std::string version = table["version"].value_or("");
            bool system = table["system"].value_or(false);
            deps.emplace_back(dep_name, version, system);
            break;
        }
        case toml::node_type::string: {
            std::string version = value.as_string()->get();
            deps.emplace_back(dep_name, version, false);
            break;
        }
        default:
            std::cerr << "error: invalid value for dependency '" << dep_name << "'\n";
            return 1;
        }
    }

    std::vector<std::string> import_strings;
    for (auto &dep : deps) {
        if (dep.build(paths, out))
            return 1;
        std::filesystem::path output_path = make_output_path(out, dep.name, dep.version);
        output_path.replace_extension(".tlib");
        import_strings.emplace_back("-I");
        import_strings.push_back(output_path.string());
    }

    std::filesystem::path out_path = make_output_path(out, name, version);
    std::filesystem::create_directories(out_path.parent_path());

    std::filesystem::path root_src = in / "src";
    std::string library_str;

    if (exists(root_src / "lib.tk")) {
        root_src /= "lib.tk";
        library_str = "--emit=lib";
    } else if (exists(root_src / "main.tk")) {
        root_src /= "main.tk";
    } else {
        std::cerr << "error: neither 'src/main.tk' nor 'src/lib.tk' exist\n";
        return 1;
    }

    std::vector<std::string> args;

    args.push_back(paths.tarik.string());
    args.insert(args.end(), import_strings.begin(), import_strings.end());
    args.emplace_back("--emit=obj");
    if (!library_str.empty()) {
        args.push_back(library_str);
    }
    args.push_back("--output=" + out_path.string());
    args.push_back(root_src.string());

    if (volatile_) {
        std::cerr << " * Executing '\"" << paths.tarik.string() << "\" ";
        for (auto &arg : args) {
            std::cerr << "\"" << arg << "\" ";
        }
        std::cerr << "\b'\n";
    }

    std::vector<llvm::StringRef> args_ref;

    args_ref.reserve(args.size());
    for (auto &arg : args) {
        args_ref.emplace_back(arg);
    }

    int ret = llvm::sys::ExecuteAndWait(paths.tarik.string(), llvm::ArrayRef(args_ref.data(), args_ref.size()));
    if (ret)
        return ret;

    return 0;
}

int main(int argc, const char *argv[]) {
    ArgumentParser parser(argc, argv, "temet");

    Option *version = parser.add_option("version", "Miscellaneous", "Display the compiler version");
    Option *volatile_option = parser.add_option("volatile",
                                                "Miscellaneous",
                                                "Print details about what temet is doing",
                                                'v');

    for (const auto &option : parser) {
        if (option == version) {
            std::cout << version_id << " temet version " << version_string << "\n";
            return 0;
        } else if (option == volatile_option) {
            volatile_ = true;
        }
    }

    Paths paths(argv[0]);

    if (paths.tarik.empty()) {
        std::cerr << "error: couldn't find tarik\n";
        return 1;
    }

    std::vector<std::string> inputs = parser.get_inputs();

    if (inputs.empty()) {
        std::cerr << "error: No command\n";
        return 1;
    }

    if (inputs[0] == "build") {
        int ret = build(paths, std::filesystem::current_path(), std::filesystem::current_path() / "target");
        if (volatile_) {
            std::cerr << " * Exiting " << std::filesystem::current_path() << "\n";
        }
        return ret;
    }
    return 0;
}

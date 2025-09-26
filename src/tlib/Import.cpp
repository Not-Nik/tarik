// tarik (c) Nikolas Wipper 2025

#include "Import.h"

#include <fstream>
#include <unordered_set>

#include "Deserialise.h"

std::vector<aast::Statement *> import_statements(std::filesystem::path path) {
    std::ifstream file(path);

    char MAGIC[4];
    file.read(MAGIC, 4);

    if (memcmp(MAGIC, "TLIB", 4) != 0) {
        return {};
    }

    return deserialise(file);
}

void add_seen_if_path(Path &path, std::unordered_set<std::string> &seen_roots) {
    std::vector<std::string> parts = path.get_parts();
    if (parts.size() > 1)
        seen_roots.insert(parts[0]);
}

void add_prefix_if_seen(Path &path, std::unordered_set<std::string> seen_roots, const Path &prefix) {
    std::vector<std::string> parts = path.get_parts();
    if (parts.size() == 1 || seen_roots.contains(parts[0])) {
        path = path.with_prefix(prefix);
    }
}

void lift_up_undefined(std::vector<aast::Statement *> &statements, const Path &prefix) {
    std::unordered_set<std::string> seen_roots;
    for (aast::Statement *stmt : statements) {
        switch (stmt->statement_type) {
        case aast::FUNC_DECL_STMT: {
            add_seen_if_path(((aast::FuncDeclareStatement *) stmt)->path, seen_roots);
            break;
        }
        case aast::STRUCT_STMT: {
            add_seen_if_path(((aast::StructStatement *) stmt)->path, seen_roots);
            break;
        }
        default:
            break;
        }
    }

    for (aast::Statement *stmt : statements) {
        switch (stmt->statement_type) {
        case aast::FUNC_DECL_STMT: {
            auto func = (aast::FuncDeclareStatement *) stmt;
            add_prefix_if_seen(func->path, seen_roots, prefix);
            if (!func->return_type.is_primitive()) {
                add_prefix_if_seen(func->return_type.get_user(), seen_roots, prefix);
            }
            for (auto *arg : func->arguments) {
                if (!arg->type.is_primitive()) {
                    add_prefix_if_seen(arg->type.get_user(), seen_roots, prefix);
                }
            }
            break;
        }
        case aast::STRUCT_STMT: {
            auto str = (aast::StructStatement *) stmt;
            add_prefix_if_seen(str->path, seen_roots, prefix);
            for (auto *arg : str->members) {
                if (!arg->type.is_primitive()) {
                    add_prefix_if_seen(arg->type.get_user(), seen_roots, prefix);
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

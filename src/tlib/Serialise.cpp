// tarik (c) Nikolas Wipper 2025

#include "Serialise.h"

template <>
void serialise(std::ostream &os, std::vector<aast::Statement *> vec) {
    size_t i = 0;
    std::stringstream buffer;
    for (auto el : vec) {
        auto old_size = buffer.tellp();
        serialise(buffer, el);
        if (old_size < buffer.tellp()) {
            i++;
        }
    }
    serialise(os, i);
    os << buffer.rdbuf();
}

void serialise(std::ostream &os, size_t siz) {
    os.write(reinterpret_cast<const char *>(&siz), sizeof(siz));
}

void serialise(std::ostream &os, bool bol) {
    uint8_t byte = bol;
    os.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
}

void serialise(std::ostream &os, const std::string &string) {
    serialise(os, string.size());
    os.write(string.data(), string.size());
}

void serialise(std::ostream &os, const LexerRange &range) {
    serialise(os, range.filename.string());
    serialise(os, (size_t) range.l);
    serialise(os, (size_t) range.p);
    serialise(os, (size_t) range.length);
}

void serialise(std::ostream &os, Path path) {
    // is_global isn't necessary, because analysed paths are always global
    serialise(os, path.get_parts());
}

void serialise(std::ostream &os, Type type) {
    serialise(os, (size_t) type.pointer_level);
    if (type.is_primitive()) {
        serialise(os, true);
        serialise(os, (size_t) type.get_primitive());
    } else {
        serialise(os, false);
        serialise(os, type.get_user());
    }
}

void serialise(std::ostream &os, Token token) {
    serialise(os, token.origin);
    serialise(os, token.raw);
}

void serialise(std::ostream &os, aast::VariableStatement *decl) {
    serialise(os, decl->origin);
    serialise(os, decl->type);
    serialise(os, decl->name);
}

void serialise(std::ostream &os, aast::FuncDeclareStatement *decl) {
    serialise(os, (size_t) decl->statement_type);
    serialise(os, decl->origin);
    serialise(os, decl->path);
    serialise(os, decl->return_type);
    serialise(os, decl->arguments);
    serialise(os, decl->var_arg);
}

void serialise(std::ostream &os, aast::StructStatement *decl) {
    serialise(os, (size_t) decl->statement_type);
    serialise(os, decl->origin);
    serialise(os, decl->path);
    serialise(os, decl->members);
}

void serialise(std::ostream &os, aast::ImportStatement *imp) {
    if (imp->local) {
        serialise(os, imp->block);
    }
}

void serialise(std::ostream &os, aast::Statement *decl) {
    switch (decl->statement_type) {
    case aast::FUNC_DECL_STMT:
        serialise(os, (aast::FuncDeclareStatement *) decl);
        break;
    case aast::STRUCT_STMT:
        serialise(os, (aast::StructStatement *) decl);
        break;
    case aast::IMPORT_STMT:
        serialise(os, (aast::ImportStatement *) decl);
        break;
    default:
        break;
    }
}

// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_CODEGEN_LLVM_H_
#define TARIK_SRC_CODEGEN_LLVM_H_

#include <map>
#include <string>
#include <filesystem>

#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>

#include "semantic/ast/Statements.h"
#include "semantic/ast/Expression.h"

class LLVM {
    std::unique_ptr<llvm::Module> module;
    llvm::Type *return_type = nullptr;
    bool return_type_signed_int = false;
    llvm::Function *current_function = nullptr;
    llvm::BasicBlock *last_loop_entry = nullptr, *last_loop_exit = nullptr;
    std::map<std::string, llvm::FunctionType *> functions;
    std::map<std::string, llvm::Function *> function_bodies;
    std::map<std::string, std::tuple<llvm::Value *, llvm::Type *, bool>> variables;
    std::map<std::string, llvm::StructType *> structures;
    std::map<std::string, aast::StructStatement *> struct_statements;

public:
    static inline std::string default_triple = llvm::sys::getDefaultTargetTriple();

    struct Config {
        std::string triple = default_triple;

        enum class Output {
            Assembly,
            Object
        } output = Output::Object;

        llvm::CodeGenOptLevel optimisation_level = llvm::CodeGenOptLevel::None;
        bool pic = false;
        std::optional<llvm::CodeModel::Model> code_model;
    };

    explicit LLVM(const std::string &name);
    static void force_init();

    int dump_ir(const std::string &to);
    int write_file(const std::string &to, Config config);

    void generate_statement(aast::Statement *s, bool is_last);
    void generate_statements(const std::vector<aast::Statement *> &s, bool is_last = true);

    static bool is_valid_triple(const std::string &triple) {
        std::string err;
        return llvm::TargetRegistry::lookupTarget(triple, err) != nullptr;
    }

    static std::vector<std::string> get_available_triples() {
        std::vector<std::string> res;
        for (auto t : llvm::TargetRegistry::targets())
            res.emplace_back(t.getName());
        std::reverse(res.begin(), res.end());
        return res;
    }

protected:
    void generate_scope(aast::ScopeStatement *scope, bool is_last);
    void generate_function(aast::FuncStatement *func);
    void generate_func_decl(aast::FuncDeclareStatement *decl);
    void generate_if(aast::IfStatement *if_, bool is_last);
    void generate_else(aast::ElseStatement *else_);
    void generate_return(aast::ReturnStatement *return_);
    void generate_while(aast::WhileStatement *while_, bool is_last);
    void generate_break(aast::BreakStatement *break_);
    void generate_continue(aast::ContinueStatement *continue_);
    void generate_variable(aast::VariableStatement *var);
    void generate_struct(aast::StructStatement *struct_);
    void generate_import(aast::ImportStatement *import_, bool is_last);
    llvm::Value *generate_expression(aast::Expression *expression);
    static llvm::Value *generate_cast(llvm::Value *val, llvm::Type *type, bool signed_int = true);

    std::tuple<llvm::Value *, llvm::Type *, bool> get_var_on_stack(std::string name);
    llvm::Type *make_llvm_type(const Type &t);
    llvm::FunctionType *make_llvm_function_type(aast::FuncStCommon *func);
    llvm::Value *generate_member_access(aast::BinaryExpression *mae);
};

#endif //TARIK_SRC_CODEGEN_LLVM_H_

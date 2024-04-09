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

#include "syntactic/expressions/Statements.h"
#include "syntactic/expressions/Expression.h"

class LLVM {
    std::unique_ptr<llvm::Module> module;
    llvm::Type *return_type = nullptr;
    bool return_type_signed_int = false;
    llvm::Function *current_function = nullptr;
    llvm::BasicBlock *last_loop_entry = nullptr, *last_loop_exit = nullptr;
    std::map<std::string, llvm::FunctionType *> functions;
    std::map<std::string, llvm::Function *> function_bodies;
    std::map<std::string, std::pair<llvm::Value *, llvm::Type *>> variables;
    std::map<std::string, llvm::StructType *> structures;
    std::map<std::string, StructStatement *> struct_statements;

public:
    static inline std::string default_triple = llvm::sys::getDefaultTargetTriple();

    struct Config {
        std::string triple = default_triple;

        enum class Output {
            Assembly,
            Object
        } output = Output::Object;

        llvm::CodeGenOpt::Level optimisation_level = llvm::CodeGenOpt::None;
        bool pic = false;
        std::optional<llvm::CodeModel::Model> code_model;
    };

    explicit LLVM(const std::string &name);
    static void force_init();

    int dump_ir(const std::string &to);
    int write_file(const std::string &to, Config config);

    void generate_statement(Statement *s, bool is_last);
    void generate_statements(const std::vector<Statement *> &s, bool is_last = true);

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
    void generate_scope(ScopeStatement *scope, bool is_last);
    void generate_function(FuncStatement *func);
    void generate_func_decl(FuncDeclareStatement *decl);
    void generate_if(IfStatement *if_, bool is_last);
    void generate_else(ElseStatement *else_);
    void generate_return(ReturnStatement *return_);
    void generate_while(WhileStatement *while_, bool is_last);
    void generate_break(BreakStatement *break_);
    void generate_continue(ContinueStatement *continue_);
    void generate_variable(VariableStatement *var);
    void generate_struct(StructStatement *struct_);
    void generate_import(ImportStatement *import_, bool is_last);
    llvm::Value *generate_expression(Expression *expression);
    static llvm::Value *generate_cast(llvm::Value *val, llvm::Type *type, bool signed_int = true);

    llvm::Type *make_llvm_type(const Type &t);
    llvm::FunctionType *make_llvm_function_type(FuncStCommon *func);
    llvm::Value *generate_member_access(BinaryOperatorExpression *mae);
};

#endif //TARIK_SRC_CODEGEN_LLVM_H_

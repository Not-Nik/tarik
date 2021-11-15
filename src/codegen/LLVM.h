// tarik (c) Nikolas Wipper 2021

#ifndef TARIK_SRC_CODEGEN_LLVM_H_
#define TARIK_SRC_CODEGEN_LLVM_H_

#include <map>
#include <string>
#include <memory>
#include <filesystem>

#include <llvm/Config/llvm-config.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <llvm/Support/raw_os_ostream.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"

#include "syntactic/expressions/Statements.h"
#include "syntactic/expressions/Expression.h"

class LLVM {
    std::unique_ptr<llvm::Module> module;
    llvm::Type *return_type;
    bool return_type_signed_int;
    llvm::Function *current_function;
    llvm::BasicBlock *last_loop_entry = nullptr, *last_loop_exit = nullptr;
    std::map<std::string, llvm::FunctionType *> functions;
    std::map<std::string, std::pair<llvm::Value *, llvm::Type *>> variables;
public:
    explicit LLVM(const std::string &name);
    static void force_init();

    void dump_ir(const std::string &to);
    void write_object_file(const std::string &to, const std::string &triple = default_triple);

    void generate_statement(Statement *s);
    void generate_statements(const std::vector<Statement *> &s);

    static inline std::string default_triple = llvm::sys::getDefaultTargetTriple();
    static bool is_valid_triple(const std::string &triple) {
        std::string err;
        return llvm::TargetRegistry::lookupTarget(triple, err) != nullptr;
    }
    static std::vector<std::string> get_available_triples() {
        std::vector<std::string> res;
        for (auto t: llvm::TargetRegistry::targets()) res.emplace_back(t.getName());
        std::reverse(res.begin(), res.end());
        return res;
    }
protected:
    void generate_scope(ScopeStatement *scope);
    void generate_function(FuncStatement *func);
    void generate_if(IfStatement *if_);
    void generate_else(ElseStatement *else_);
    void generate_return(ReturnStatement *return_);
    void generate_while(WhileStatement *while_);
    void generate_break(BreakStatement *break_);
    void generate_continue(ContinueStatement *continue_);
    void generate_variable(VariableStatement *var);
    void generate_struct(StructStatement *struct_);
    llvm::Value *generate_expression(Expression *expression);
    static llvm::Value *generate_cast(llvm::Value *val, llvm::Type *type, bool signed_int = true);

    static llvm::Type *make_llvm_type(const Type &t);
    static llvm::FunctionType *make_llvm_function_type(FuncStatement *func);
};

#endif //TARIK_SRC_CODEGEN_LLVM_H_

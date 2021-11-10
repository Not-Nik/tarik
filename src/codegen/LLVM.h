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

class LLVM {
    std::unique_ptr<llvm::Module> module;
    std::map<std::string, llvm::Value *> variables;
public:
    explicit LLVM(const std::string &name);

    void dump_ir(const std::string &to);
    void write_object_file(const std::string &to, const std::string &triple = llvm::sys::getDefaultTargetTriple());

    void generate_statement(Statement *s);
    void generate_statements(const std::vector<Statement *> &s);

protected:
    void generate_scope(ScopeStatement *scope);
    void generate_function(FuncStatement *func);

    static llvm::Type *make_llvm_type(const Type &t);
    static llvm::FunctionType *make_llvm_function_type(FuncStatement *func);
};

#endif //TARIK_SRC_CODEGEN_LLVM_H_

// tarik (c) Nikolas Wipper 2021

#include "LLVM.h"

#include <sstream>
#include <iostream>

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);

LLVM::LLVM(const std::string &name)
    : module(std::make_unique<llvm::Module>(name, context)) {
    static bool initialised = false;
    if (!initialised) {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        initialised = true;
    }
}

void LLVM::dump_ir(const std::string &to) {
    std::error_code EC;
    llvm::raw_fd_ostream stream(to, EC, llvm::sys::fs::CD_CreateAlways);
    if (!EC) {
        module->print(stream, nullptr);
    }
}

void LLVM::write_object_file(const std::string &to, const std::string &triple) {
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(triple, error);

    if (!target) {
        // probably an invalid triple
        std::cerr << "unknown triple '" << triple << "'\n";
        return;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto target_machine = target->createTargetMachine(triple, cpu, features, opt, rm);

    module->setDataLayout(target_machine->createDataLayout());
    module->setTargetTriple(triple);

    std::error_code EC;
    llvm::raw_fd_ostream stream(to, EC, llvm::sys::fs::CD_CreateAlways);

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, stream, nullptr, FileType)) {
        // damn
        std::cerr << "couldn't open '" << to << "'\n";
        return;
    }

    pass.run(*module);
    stream.flush();
}

void LLVM::generate_statement(Statement *statement) {
    switch (statement->statement_type) {
        case SCOPE_STMT:
            generate_scope((ScopeStatement *) statement);
            break;
        case FUNC_STMT:
            generate_function((FuncStatement *) statement);
            break;
        case IF_STMT:
            break;
        case ELSE_STMT:
            break;
        case RETURN_STMT:
            break;
        case WHILE_STMT:
            break;
        case BREAK_STMT:
            break;
        case CONTINUE_STMT:
            break;
        case VARIABLE_STMT:
            break;
        case STRUCT_STMT:
            break;
        case EXPR_STMT:
            break;
    }
}

void LLVM::generate_statements(const std::vector<Statement *> &statements) {
    for (auto statement: statements) {
        generate_statement(statement);
    }
}

void LLVM::generate_scope(ScopeStatement *scope) {
    generate_statements(scope->block);
}

void LLVM::generate_function(FuncStatement *func) {
    llvm::FunctionType *func_type = make_llvm_function_type(func);

    llvm::Function *llvm_func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, func->name, module.get());
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", llvm_func);
    builder.SetInsertPoint(entry);

    auto it = func->arguments.begin();
    for (auto &arg: llvm_func->args()) {
        auto arg_var = builder.CreateAlloca(arg.getType(), unsigned(0));
        arg_var->setName((*it)->name);
        builder.CreateStore(&arg, arg_var);
    }

    generate_scope(func);

    // TODO: check if function has a return statement
    // Note: we already check for that, but don't generate returns yet, so this is in the commit to generate valid LLVM ir
    llvm::Value *zero_ret = llvm::ConstantInt::get(func_type->getReturnType(), 0);
    builder.CreateRet(zero_ret);
}

llvm::Type *LLVM::make_llvm_type(const Type &t) {
    llvm::Type *res;
    if (t.is_primitive) {
        switch (t.type.size) {
            case U8:
            case I8:
                res = llvm::Type::getInt8Ty(context);
                break;
            case U16:
            case I16:
                res = llvm::Type::getInt16Ty(context);
                break;
            case U32:
            case I32:
                res = llvm::Type::getInt32Ty(context);
                break;
            case U64:
            case I64:
                res = llvm::Type::getInt64Ty(context);
                break;
            case F32:
                res = llvm::Type::getFloatTy(context);
                break;
            case F64:
                res = llvm::Type::getDoubleTy(context);
                break;
        }
    }

    for (int i = 0; i < t.pointer_level; i++) {
        res = res->getPointerTo();
    }

    return res;
}

llvm::FunctionType *LLVM::make_llvm_function_type(FuncStatement *func) {
    llvm::Type *return_type = make_llvm_type(func->return_type);
    std::vector<llvm::Type *> argument_types;
    for (auto arg: func->arguments) {
        argument_types.push_back(make_llvm_type(arg->type));
    }
    llvm::FunctionType *func_type = llvm::FunctionType::get(return_type, argument_types, false);
    return func_type;
}

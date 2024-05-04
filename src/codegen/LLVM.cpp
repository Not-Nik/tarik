// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "LLVM.h"

#include <bit>
#include <vector>
#include <sstream>
#include <iostream>
#include <optional>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>

static llvm::LLVMContext context;
static llvm::IRBuilder builder(context);

LLVM::LLVM(const std::string &name)
    : module(std::make_unique<llvm::Module>(name, context)) {
    force_init();
}

void LLVM::force_init() {
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

int LLVM::dump_ir(const std::string &to) {
    std::error_code EC;
    llvm::raw_fd_ostream stream(to, EC, llvm::sys::fs::CD_CreateAlways);
    if (!EC) {
        module->print(stream, nullptr);
        return 0;
    } else {
        std::cerr << "error: " << EC.message() << "\n";
        return 1;
    }
}

int LLVM::write_file(const std::string &to, Config config) {
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(config.triple, error);

    if (!target) {
        // probably an invalid triple
        std::cerr << "unknown triple '" << config.triple << "'\n";
        return 1;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto target_machine = target->createTargetMachine(config.triple,
                                                      cpu,
                                                      features,
                                                      opt,
                                                      config.pic
                                                      ? llvm::Reloc::Model::PIC_
                                                      : llvm::Reloc::Model::Static,
                                                      config.code_model,
                                                      config.optimisation_level);

    module->setDataLayout(target_machine->createDataLayout());
    module->setTargetTriple(config.triple);

    std::error_code EC;
    llvm::raw_fd_ostream stream(to, EC, llvm::sys::fs::CD_CreateAlways);

    llvm::legacy::PassManager pass;
    auto file_type = config.output == Config::Output::Assembly
                     ? llvm::CodeGenFileType::AssemblyFile
                     : llvm::CodeGenFileType::ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, stream, nullptr, file_type)) {
        // damn
        std::cerr << "couldn't open '" << to << "'\n";
        return 1;
    }

    pass.run(*module);
    stream.flush();

    return 0;
}

void LLVM::generate_statement(aast::Statement *statement, bool is_last) {
    switch (statement->statement_type) {
        case aast::SCOPE_STMT:
            generate_scope((aast::ScopeStatement *) statement, is_last);
            break;
        case aast::FUNC_STMT:
            generate_function((aast::FuncStatement *) statement);
            break;
        case aast::FUNC_DECL_STMT:
            generate_func_decl((aast::FuncDeclareStatement *) statement);
            break;
        case aast::IF_STMT:
            generate_if((aast::IfStatement *) statement, is_last);
            break;
        case aast::ELSE_STMT:
            generate_else((aast::ElseStatement *) statement);
            break;
        case aast::RETURN_STMT:
            generate_return((aast::ReturnStatement *) statement);
            break;
        case aast::WHILE_STMT:
            generate_while((aast::WhileStatement *) statement, is_last);
            break;
        case aast::BREAK_STMT:
            generate_break((aast::BreakStatement *) statement);
            break;
        case aast::CONTINUE_STMT:
            generate_continue((aast::ContinueStatement *) statement);
            break;
        case aast::VARIABLE_STMT:
            generate_variable((aast::VariableStatement *) statement);
            break;
        case aast::STRUCT_STMT:
            generate_struct((aast::StructStatement *) statement);
            break;
        case aast::IMPORT_STMT:
            generate_import((aast::ImportStatement *) statement, is_last);
            break;
        case aast::EXPR_STMT:
            generate_expression((aast::Expression *) statement);
            break;
    }
}

void LLVM::generate_statements(const std::vector<aast::Statement *> &statements, bool is_last) {
    for (auto it = statements.begin(); it != statements.end(); ++it) {
        generate_statement(*it, is_last && it + 1 == statements.end());
    }
}

void LLVM::generate_scope(aast::ScopeStatement *scope, bool is_last) {
    generate_statements(scope->block, is_last);
}

void LLVM::generate_function(aast::FuncStatement *func) {
    variables.clear();

    llvm::FunctionType *func_type = functions.at(func->path.str());

    llvm::Function *llvm_func = function_bodies.at(func->path.str());
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "func_entry", llvm_func);
    builder.SetInsertPoint(entry);
    current_function = llvm_func;

    auto it = func->arguments.begin();
    for (auto &arg : llvm_func->args()) {
        variables.emplace((*it)->name.raw, std::make_tuple(&arg, arg.getType(), true));
        ++it;
    }
    return_type = func_type->getReturnType();
    if (return_type->isIntegerTy())
        return_type_signed_int = func->return_type.is_signed_int();

    generate_scope(func, true);

    if (func->return_type == Type(VOID) && func->block.back()->statement_type != aast::RETURN_STMT) {
        builder.CreateRetVoid();
    }
}

void LLVM::generate_func_decl(aast::FuncDeclareStatement *decl) {
    llvm::FunctionType *func_type = make_llvm_function_type(decl);
    functions.emplace(decl->path.str(), func_type);

    llvm::Function *llvm_func = llvm::Function::Create(func_type,
                                                       llvm::Function::ExternalLinkage,
                                                       decl->path.str(),
                                                       module.get());
    function_bodies.emplace(decl->path.str(), llvm_func);
}

void LLVM::generate_if(aast::IfStatement *if_, bool is_last) {
    llvm::BasicBlock *if_block = llvm::BasicBlock::Create(context, "if_block", current_function);
    llvm::BasicBlock *endif_block = llvm::BasicBlock::Create(context, "endif_block");
    llvm::BasicBlock *else_block = nullptr;

    llvm::Value *condition = generate_expression(if_->condition);

    if (if_->else_statement) {
        else_block = llvm::BasicBlock::Create(context, "else_block", current_function);
        builder.CreateCondBr(condition, if_block, else_block);
    } else {
        // An if without an else can never be the last statement, because it doesn't guarantee a return statement
        builder.CreateCondBr(condition, if_block, endif_block);
    }
    builder.SetInsertPoint(if_block);

    generate_scope(if_, false);
    if (!is_last)
        builder.CreateBr(endif_block);

    if (else_block) {
        builder.SetInsertPoint(else_block);
        generate_scope(if_->else_statement, is_last);
        if (!is_last)
            builder.CreateBr(endif_block);
    }

    if (!is_last) {
        current_function->insert(current_function->end(), endif_block);
        builder.SetInsertPoint(endif_block);
    }
}

void LLVM::generate_else(aast::ElseStatement *) {
    // this will never happen
}

void LLVM::generate_return(aast::ReturnStatement *return_) {
    if (!return_type)
        return;
    if (return_->value)
        builder.CreateRet(generate_cast(generate_expression(return_->value), return_type, return_type_signed_int));
    else
        builder.CreateRetVoid();
}

void LLVM::generate_while(aast::WhileStatement *while_, bool is_last) {
    llvm::BasicBlock *while_comp_block = llvm::BasicBlock::Create(context, "while_comp_block");
    llvm::BasicBlock *while_block = llvm::BasicBlock::Create(context, "while_block");
    llvm::BasicBlock *endwhile_block = llvm::BasicBlock::Create(context, "endwhile_block");

    builder.CreateBr(while_comp_block);

    current_function->insert(current_function->end(), while_comp_block);
    builder.SetInsertPoint(while_comp_block);

    llvm::Value *condition = generate_expression(while_->condition);
    builder.CreateCondBr(condition, while_block, is_last ? nullptr : endwhile_block);

    current_function->insert(current_function->end(), while_block);
    builder.SetInsertPoint(while_block);

    llvm::BasicBlock *old_llen = last_loop_entry, *old_llex = last_loop_exit;
    last_loop_entry = while_comp_block;
    last_loop_exit = endwhile_block;

    generate_scope(while_, is_last);
    builder.CreateBr(while_comp_block);

    last_loop_entry = old_llen;
    last_loop_exit = old_llex;

    if (!is_last) {
        current_function->insert(current_function->end(), endwhile_block);
        builder.SetInsertPoint(endwhile_block);
    }
}

void LLVM::generate_break(aast::BreakStatement *) {
    builder.CreateBr(last_loop_exit);
}

void LLVM::generate_continue(aast::ContinueStatement *) {
    builder.CreateBr(last_loop_entry);
}

void LLVM::generate_variable(aast::VariableStatement *var) {
    llvm::Type *type = make_llvm_type(var->type);
    variables.emplace(var->name.raw,
                      std::make_tuple(builder.CreateAlloca(type, 0u, nullptr, var->name.raw),
                                      type,
                                      false));
}

void LLVM::generate_struct(aast::StructStatement *struct_) {
    std::vector<llvm::Type *> members;
    for (auto member : struct_->members) {
        members.push_back(make_llvm_type(member->type));
    }

    structures.emplace(struct_->path.str(), llvm::StructType::create(context, members, struct_->path.str()));
    struct_statements.emplace(struct_->path.str(), struct_);
}

void LLVM::generate_import(aast::ImportStatement *import_, bool is_last) {
    generate_statements(import_->block, is_last);
}

// https://stackoverflow.com/questions/3407012/rounding-up-to-the-nearest-multiple-of-a-number#3407254
template <std::integral T>
int roundUp(T numToRound, T multiple) {
    if (multiple == 0)
        return numToRound;

    T remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

llvm::Value *LLVM::generate_expression(aast::Expression *expression) {
    switch (expression->expression_type) {
        case aast::CALL_EXPR: {
            auto ce = (aast::CallExpression *) expression;
            llvm::FunctionCallee function;

            if (ce->callee->expression_type == aast::NAME_EXPR) {
                std::string name = ((aast::NameExpression *) ce->callee)->name;
                function = module->getOrInsertFunction(name, functions.at(name));
            } else {
                throw "__unimplemented(expression_calling)";
            }

            std::vector<llvm::Value *> arg_values;
            size_t arg_i = 0;
            for (auto arg : ce->arguments) {
                // Check if we are past the regular, non-variable arguments,
                // and don't try to cast if we are
                if (arg_i >= function.getFunctionType()->getNumParams()) {
                    arg_values.push_back(generate_expression(arg));
                } else {
                    arg_values.push_back(generate_cast(generate_expression(arg),
                                                       function.getFunctionType()->getParamType(arg_i++),
                                                       arg->type.is_signed_int()));
                }
            }
            const char *name = "";
            if (!function.getFunctionType()->getReturnType()->isVoidTy())
                name = "call_temp";
            return builder.CreateCall(function, arg_values, name);
        }
        case aast::DASH_EXPR:
        case aast::DOT_EXPR:
        case aast::EQ_EXPR:
        case aast::COMP_EXPR: {
            auto ce = (aast::BinaryExpression *) expression;

            llvm::Value *left = generate_expression(ce->left), *right = generate_expression(ce->right);
            bool unsigned_int = ce->left->type.is_unsigned_int() || ce->right->type.is_unsigned_int();
            // After validation, implicit casts are never from float to integer or vice versa
            llvm::Type *result_type = make_llvm_type(ce->left->type.get_result(ce->right->type));
            bool fp = result_type->isFloatingPointTy();

            // signed_int parameter doesn't matter, because we never cast from float to int or vice versa implicitly
            left = generate_cast(left, result_type);
            right = generate_cast(right, result_type);

            switch (ce->bin_op_type) {
                case aast::ADD:
                    if (fp)
                        return builder.CreateFAdd(left, right, "add_temp");
                    else
                        return builder.CreateAdd(left, right, "add_temp");
                case aast::SUB:
                    if (fp)
                        return builder.CreateFSub(left, right, "sub_temp");
                    else
                        return builder.CreateSub(left, right, "sub_temp");
                case aast::MUL:
                    if (fp)
                        return builder.CreateFMul(left, right, "mul_temp");
                    else
                        return builder.CreateMul(left, right, "mul_temp");
                case aast::DIV:
                    if (fp)
                        return builder.CreateFDiv(left, right, "div_temp");
                    else if (unsigned_int)
                        return builder.CreateUDiv(left, right, "div_temp");
                    else
                        return builder.CreateSDiv(left, right);
                case aast::EQ:
                    if (fp)
                        return builder.CreateFCmpOEQ(left, right, "eq_temp");
                    else
                        return builder.CreateICmpEQ(left, right, "eq_temp");
                case aast::NEQ:
                    if (fp)
                        return builder.CreateFCmpONE(left, right, "neq_temp");
                    else
                        return builder.CreateICmpNE(left, right, "neq_temp");
                case aast::SM:
                    if (fp)
                        return builder.CreateFCmpOLE(left, right, "sm_temp");
                    else if (unsigned_int)
                        return builder.CreateICmpULT(left, right, "sm_temp");
                    else
                        return builder.CreateICmpSLT(left, right, "sm_temp");
                case aast::GR:
                    if (fp)
                        return builder.CreateFCmpOGT(left, right, "gr_temp");
                    else if (unsigned_int)
                        return builder.CreateICmpUGT(left, right, "gr_temp");
                    else
                        return builder.CreateICmpSGT(left, right, "gr_temp");
                case aast::SME:
                    if (fp)
                        return builder.CreateFCmpOLE(left, right, "sme_temp");
                    else if (unsigned_int)
                        return builder.CreateICmpULE(left, right, "sme_temp");
                    else
                        return builder.CreateICmpSLE(left, right, "sme_temp");
                case aast::GRE:
                    if (fp)
                        return builder.CreateFCmpOGE(left, right, "gre_temp");
                    else if (unsigned_int)
                        return builder.CreateICmpUGE(left, right, "gre_temp");
                    else
                        return builder.CreateICmpSGE(left, right, "gre_temp");
            }
            break;
        }
        case aast::MEM_ACC_EXPR: {
            auto mae = (aast::BinaryExpression *) expression;
            llvm::Value *gep = generate_member_access(mae);
            return builder.CreateLoad(make_llvm_type(mae->type), gep, "deref_temp");
        }
        case aast::PREFIX_EXPR: {
            auto pe = (aast::PrefixExpression *) expression;

            if (pe->prefix_type == aast::REF) {
                if (pe->operand->expression_type == aast::NAME_EXPR) {
                    auto [var, type, is_arg] = get_var_on_stack(((aast::NameExpression *) pe->operand)->name);
                    return var;
                } else if (pe->operand->expression_type == aast::MEM_ACC_EXPR) {
                    return generate_member_access((aast::BinaryExpression *) expression);
                }
                llvm::Type *ty = make_llvm_type(pe->type);
                llvm::Value *alloca = builder.CreateAlloca(ty, 0u, nullptr, "_ref_temp");
                builder.CreateStore(generate_expression(pe->operand), alloca);
                return alloca;
            }

            llvm::Value *val = generate_expression(pe->operand);

            switch (pe->prefix_type) {
                case aast::NEG:
                    if (val->getType()->isFloatingPointTy())
                        return builder.CreateFNeg(val, "neg_temp");
                    else
                        return builder.CreateNeg(val, "neg_temp");
                case aast::DEREF:
                    return builder.CreateLoad(make_llvm_type(pe->type), val, "deref_temp");
                case aast::LOG_NOT:
                    return builder.CreateNot(val, "not_temp");
                case aast::REF:
                    break;
            }
            break;
        }
        case aast::ASSIGN_EXPR: {
            auto ae = (aast::BinaryExpression *) expression;
            llvm::Value *dest;
            llvm::Type *dest_type;
            if (ae->left->expression_type == aast::NAME_EXPR) {
                auto [var, type, is_arg] = get_var_on_stack(((aast::NameExpression *) ae->left)->name);
                dest = var;
                dest_type = type;
            } else if (ae->left->expression_type == aast::MEM_ACC_EXPR) {
                dest = generate_member_access((aast::BinaryExpression *) ae->left);
                dest_type = make_llvm_type(ae->left->type);
            } else if (ae->left->expression_type == aast::PREFIX_EXPR &&
                ((aast::PrefixExpression *) ae->left)->prefix_type == aast::DEREF) {
                auto deref = (aast::PrefixExpression *) ae->left;
                dest = generate_expression(deref->operand);
                dest_type = make_llvm_type(deref->type);
            } else {
                dest = generate_expression(ae->left);
                dest_type = dest->getType();
            }
            return builder.CreateStore(generate_cast(generate_expression(ae->right), dest_type), dest);
        }
        case aast::NAME_EXPR: {
            auto ne = (aast::NameExpression *) expression;
            auto [var, type, is_arg] = variables.at(ne->name);
            if (is_arg)
                return var;
            else
                return builder.CreateLoad(type, var, "load_temp");
        }
        case aast::INT_EXPR: {
            auto ie = (aast::IntExpression *) expression;
            size_t width = std::max(8, roundUp((size_t) std::bit_width((size_t) ie->n), size_t(8)));
            return llvm::ConstantInt::get(llvm::Type::getIntNTy(context, width), ie->n, true);
        }
        case aast::REAL_EXPR: {
            auto re = (aast::RealExpression *) expression;
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), re->n);
        }
        case aast::STR_EXPR: {
            auto se = (aast::StringExpression *) expression;
            return builder.CreateGlobalStringPtr(se->n, "string_value");
        }
        case aast::BOOL_EXPR: {
            auto be = (aast::BoolExpression *) expression;
            return llvm::ConstantInt::get(llvm::Type::getIntNTy(context, 1), be->n, false);
        }
        case aast::CAST_EXPR: {
            auto ce = (aast::CastExpression *) expression;
            return generate_cast(generate_expression(ce->expression),
                                 make_llvm_type(ce->type),
                                 ce->expression->type.is_signed_int()
                                 || ce->type.is_signed_int());
        }
    }
    return nullptr;
}

llvm::Value *LLVM::generate_cast(llvm::Value *val, llvm::Type *type, bool signed_int) {
    if (val->getType() == type)
        return val;
    llvm::Instruction::CastOps co;
    if (type->isFloatingPointTy()) {
        if (val->getType()->isFloatingPointTy())
            return builder.CreateFPCast(val, type, "cast_temp");
        if (signed_int)
            co = llvm::Instruction::SIToFP;
        else
            co = llvm::Instruction::UIToFP;
    } else {
        if (!val->getType()->isFloatingPointTy())
            return builder.CreateIntCast(val, type, signed_int, "cast_temp");
        if (signed_int)
            co = llvm::Instruction::FPToSI;
        else
            co = llvm::Instruction::FPToUI;
    }
    return builder.CreateCast(co, val, type, "cast_temp");
}

std::tuple<llvm::Value *, llvm::Type *, bool> LLVM::get_var_on_stack(std::string name) {
    auto &[var, type, is_arg] = variables.at(name);

    if (is_arg) {
        auto alloca = builder.CreateAlloca(type, 0u, nullptr, "stack_" + name);
        builder.CreateStore(var, alloca);

        var = alloca;
        is_arg = false;
    }

    return std::make_tuple(var, type, false);
}

llvm::Type *LLVM::make_llvm_type(const Type &t) {
    llvm::Type *res;
    if (t.is_primitive()) {
        switch (t.get_primitive()) {
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
            case BOOL:
                res = llvm::Type::getInt1Ty(context);
                break;
            case VOID:
                res = llvm::Type::getVoidTy(context);
        }
    } else {
        res = structures.at(t.base());
    }

    for (int i = 0; i < t.pointer_level; i++) {
        res = res->getPointerTo();
    }

    return res;
}

llvm::FunctionType *LLVM::make_llvm_function_type(aast::FuncStCommon *func) {
    llvm::Type *rt = make_llvm_type(func->return_type);
    std::vector<llvm::Type *> argument_types;
    argument_types.reserve(func->arguments.size());
    for (auto arg : func->arguments) {
        argument_types.push_back(make_llvm_type(arg->type));
    }
    llvm::FunctionType *func_type = llvm::FunctionType::get(rt, argument_types, func->var_arg);
    return func_type;
}

llvm::Value *LLVM::generate_member_access(aast::BinaryExpression *mae) {
    std::string member_name = ((aast::NameExpression *) mae->right)->name;

    llvm::Value *left;
    std::string struct_;
    bool pointer = mae->left->type.pointer_level > 0;

    if (mae->left->expression_type == aast::NAME_EXPR) {
        std::string var_name = ((aast::NameExpression *) mae->left)->name;
        auto [var, type, is_arg] = variables.at(var_name);

        left = var;
        struct_ = mae->left->type.base();
        pointer = pointer && !is_arg;
    } else {
        left = generate_expression(mae->left);
        struct_ = mae->left->type.base();
    }
    llvm::Type *struct_type = structures.at(struct_);
    unsigned int member_index = struct_statements.at(struct_)->get_member_index(member_name);

    if (!left->getType()->isPointerTy()) {
        llvm::Value *instance = builder.CreateAlloca(struct_type, 0u, nullptr, "instance_temp");
        builder.CreateStore(left, instance);
        left = instance;
    } else if (pointer) {
        left = builder.CreateLoad(struct_type->getPointerTo(), left, "deref_temp");
    }

    return builder.CreateStructGEP(struct_type, left, member_index, "member_load_temp");
}

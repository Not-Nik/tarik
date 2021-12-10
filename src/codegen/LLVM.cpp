// tarik (c) Nikolas Wipper 2021

#include "LLVM.h"

#include <bit>
#include <vector>
#include <sstream>
#include <iostream>
#include <concepts>

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
static llvm::IRBuilder<> builder(context);

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
    auto file_type = llvm::CGFT_ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, stream, nullptr, file_type)) {
        // damn
        std::cerr << "couldn't open '" << to << "'\n";
        return;
    }

    pass.run(*module);
    stream.flush();
}

void LLVM::generate_statement(Statement *statement, bool is_last) {
    switch (statement->statement_type) {
        case SCOPE_STMT:
            generate_scope((ScopeStatement *) statement, is_last);
            break;
        case FUNC_STMT:
            generate_function((FuncStatement *) statement);
            break;
        case FUNC_DECL_STMT:
            generate_func_decl((FuncDeclareStatement *) statement);
            break;
        case IF_STMT:
            generate_if((IfStatement *) statement, is_last);
            break;
        case ELSE_STMT:
            generate_else((ElseStatement *) statement);
            break;
        case RETURN_STMT:
            generate_return((ReturnStatement *) statement);
            break;
        case WHILE_STMT:
            generate_while((WhileStatement *) statement, is_last);
            break;
        case BREAK_STMT:
            generate_break((BreakStatement *) statement);
            break;
        case CONTINUE_STMT:
            generate_continue((ContinueStatement *) statement);
            break;
        case VARIABLE_STMT:
            generate_variable((VariableStatement *) statement);
            break;
        case STRUCT_STMT:
            generate_struct((StructStatement *) statement);
            break;
        case IMPORT_STMT:
            generate_import((ImportStatement *) statement, is_last);
            break;
        case EXPR_STMT:
            generate_expression((Expression *) statement);
            break;
    }
}

void LLVM::generate_statements(const std::vector<Statement *> &statements, bool is_last) {
    for (auto it = statements.begin(); it != statements.end(); ++it) {
        generate_statement(*it, is_last && it + 1 == statements.end());
    }
}

void LLVM::generate_scope(ScopeStatement *scope, bool is_last) {
    generate_statements(scope->block, is_last);
}

void LLVM::generate_function(FuncStatement *func) {
    variables.clear();

    llvm::FunctionType *func_type = make_llvm_function_type(func);

    llvm::Function *llvm_func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, func->name, module.get());
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "func_entry", llvm_func);
    builder.SetInsertPoint(entry);
    current_function = llvm_func;

    auto it = func->arguments.begin();
    for (auto &arg: llvm_func->args()) {
        arg.setName("arg_" + (*it)->name);
        auto arg_var = builder.CreateAlloca(arg.getType(), unsigned(0), nullptr, "stack_" + (*it)->name);
        builder.CreateStore(&arg, arg_var);
        variables.emplace((*it)->name, std::make_pair(arg_var, arg.getType()));
        it++;
    }
    return_type = func_type->getReturnType();
    if (return_type->isIntegerTy())
        return_type_signed_int = func->return_type.is_signed_int();
    functions.emplace(func->name, func_type);

    generate_scope(func, true);

    if (func->return_type == Type(VOID)) {
        builder.CreateRetVoid();
    }
}

bool LLVM::generate_func_decl(FuncDeclareStatement *decl) {
    llvm::FunctionType *func_type = make_llvm_function_type(decl);
    functions.emplace(decl->name, func_type);
    return false;
}

void LLVM::generate_if(IfStatement *if_, bool is_last) {
    llvm::BasicBlock *if_block = llvm::BasicBlock::Create(context, "if_block", current_function);
    llvm::BasicBlock *endif_block = llvm::BasicBlock::Create(context, "endif_block");
    llvm::BasicBlock *else_block = nullptr;
    // Todo: this should probably compare to zero instead of casting
    llvm::Value *condition = generate_cast(generate_expression(if_->condition), llvm::Type::getIntNTy(context, 1), false);

    if (if_->else_statement) {
        else_block = llvm::BasicBlock::Create(context, "else_block", current_function);
        builder.CreateCondBr(condition, if_block, else_block);
    } else {
        // An if without an else can never be the last statement, because it doesn't guarantee a return statement
        builder.CreateCondBr(condition, if_block, endif_block);
    }
    builder.SetInsertPoint(if_block);

    generate_scope(if_, false);
    if (!is_last) builder.CreateBr(endif_block);

    if (else_block) {
        builder.SetInsertPoint(else_block);
        generate_scope(if_->else_statement, is_last);
        if (!is_last) builder.CreateBr(endif_block);
    }

    if (!is_last) {
        current_function->getBasicBlockList().push_back(endif_block);
        builder.SetInsertPoint(endif_block);
    }
}

void LLVM::generate_else(ElseStatement *) {} // this will never happen

void LLVM::generate_return(ReturnStatement *return_) {
    if (!return_type) return;
    builder.CreateRet(generate_cast(generate_expression(return_->value), return_type, return_type_signed_int));
}

void LLVM::generate_while(WhileStatement *while_, bool is_last) {
    llvm::BasicBlock *while_comp_block = llvm::BasicBlock::Create(context, "while_comp_block");
    llvm::BasicBlock *while_block = llvm::BasicBlock::Create(context, "while_block");
    llvm::BasicBlock *endwhile_block = llvm::BasicBlock::Create(context, "endwhile_block");

    builder.CreateBr(while_comp_block);

    current_function->getBasicBlockList().push_back(while_comp_block);
    builder.SetInsertPoint(while_comp_block);

    // Todo: this should probably compare to zero instead of casting
    llvm::Value *condition = generate_cast(generate_expression(while_->condition), llvm::Type::getIntNTy(context, 1), false);
    builder.CreateCondBr(condition, while_block, is_last ? nullptr : endwhile_block);

    current_function->getBasicBlockList().push_back(while_block);
    builder.SetInsertPoint(while_block);

    llvm::BasicBlock *old_llen = last_loop_entry, *old_llex = last_loop_exit;
    last_loop_entry = while_comp_block;
    last_loop_exit = endwhile_block;

    generate_scope(while_, is_last);
    builder.CreateBr(while_comp_block);

    last_loop_entry = old_llen;
    last_loop_exit = old_llex;

    if (!is_last) {
        current_function->getBasicBlockList().push_back(endwhile_block);
        builder.SetInsertPoint(endwhile_block);
    }
}

void LLVM::generate_break(BreakStatement *) {
    builder.CreateBr(last_loop_exit);
}

void LLVM::generate_continue(ContinueStatement *) {
    builder.CreateBr(last_loop_entry);
}

void LLVM::generate_variable(VariableStatement *var) {
    llvm::Type *type = make_llvm_type(var->type);
    variables.emplace(var->name, std::make_pair(builder.CreateAlloca(type, unsigned(0), nullptr, var->name), type));
}

void LLVM::generate_struct(StructStatement *struct_) {
    std::vector<llvm::Type *> members;
    for (auto member: struct_->members) {
        members.push_back(make_llvm_type(member->type));
    }

    structures.emplace(struct_, llvm::StructType::create(context, members, struct_->name));
}

void LLVM::generate_import(ImportStatement *import, bool is_last) {
    generate_statements(import->block, is_last);
}

// https://stackoverflow.com/questions/3407012/rounding-up-to-the-nearest-multiple-of-a-number#3407254
template <std::integral T> int roundUp(T numToRound, T multiple) {
    if (multiple == 0) return numToRound;

    T remainder = numToRound % multiple;
    if (remainder == 0) return numToRound;

    return numToRound + multiple - remainder;
}

llvm::Value *LLVM::generate_expression(Expression *expression) {
    switch (expression->expression_type) {
        case CALL_EXPR: {
            auto ce = (CallExpression *) expression;
            llvm::FunctionCallee function;

            if (ce->callee->expression_type == NAME_EXPR) {
                std::string name = ((NameExpression *) ce->callee)->name;
                function = module->getOrInsertFunction(name, functions.at(name));
            } else {
                throw "__unimplemented(expression_calling)";
            }

            std::vector<llvm::Value *> arg_values;
            size_t arg_i = 0;
            for (auto arg: ce->arguments) {
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
            if (!function.getFunctionType()->getReturnType()->isVoidTy()) name = "call_temp";
            return builder.CreateCall(function, arg_values, name);
        }
        case DASH_EXPR:
        case DOT_EXPR:
        case EQ_EXPR:
        case COMP_EXPR: {
            auto ce = (BinaryOperatorExpression *) expression;
            bool fp = false;
            llvm::Value *left, *right;
            left = generate_expression(ce->left);
            right = generate_expression(ce->right);
            bool unsigned_int = ce->left->type.is_unsigned_int() || ce->right->type.is_unsigned_int();
            if (left->getType()->isFloatingPointTy() && !right->getType()->isFloatingPointTy()) {
                right = generate_cast(right, left->getType(), ce->right->type.is_signed_int());
                fp = true;
            } else if (right->getType()->isFloatingPointTy()) {
                left = generate_cast(left, right->getType(), ce->left->type.is_signed_int());
                fp = true;
            } else {
                if (left->getType()->getIntegerBitWidth() > right->getType()->getIntegerBitWidth())
                    right = generate_cast(right, left->getType(), ce->left->type.is_signed_int());
                else
                    left = generate_cast(left, right->getType(), ce->right->type.is_signed_int());
            }
            switch (ce->bin_op_type) {
                case ADD:
                    if (fp) return builder.CreateFAdd(left, right, "add_temp");
                    else return builder.CreateAdd(left, right, "add_temp");
                case SUB:
                    if (fp) return builder.CreateFSub(left, right, "sub_temp");
                    else return builder.CreateSub(left, right, "sub_temp");
                case MUL:
                    if (fp) return builder.CreateFMul(left, right, "mul_temp");
                    else return builder.CreateMul(left, right, "mul_temp");
                case DIV:
                    if (fp) return builder.CreateFDiv(left, right, "div_temp");
                    else if (unsigned_int) return builder.CreateUDiv(left, right, "div_temp");
                    else return builder.CreateSDiv(left, right);
                case EQ:
                    if (fp) return builder.CreateFCmpOEQ(left, right, "eq_temp");
                    else return builder.CreateICmpEQ(left, right, "eq_temp");
                case NEQ:
                    if (fp) return builder.CreateFCmpONE(left, right, "neq_temp");
                    else return builder.CreateICmpNE(left, right, "neq_temp");
                case SM:
                    if (fp) return builder.CreateFCmpOLE(left, right, "sm_temp");
                    else if (unsigned_int) return builder.CreateICmpULT(left, right, "sm_temp");
                    else return builder.CreateICmpSLT(left, right, "sm_temp");
                case GR:
                    if (fp) return builder.CreateFCmpOGT(left, right, "gr_temp");
                    else if (unsigned_int) return builder.CreateICmpUGT(left, right, "gr_temp");
                    else return builder.CreateICmpSGT(left, right, "gr_temp");
                case SME:
                    if (fp) return builder.CreateFCmpOLE(left, right, "sme_temp");
                    else if (unsigned_int) return builder.CreateICmpULE(left, right, "sme_temp");
                    else return builder.CreateICmpSLE(left, right, "sme_temp");
                case GRE:
                    if (fp) return builder.CreateFCmpOGE(left, right, "gre_temp");
                    else if (unsigned_int) return builder.CreateICmpUGE(left, right, "gre_temp");
                    else return builder.CreateICmpSGE(left, right, "gre_temp");
            }
            break;
        }
        case MEM_ACC_EXPR: {
            auto mae = (BinaryOperatorExpression *) expression;
            llvm::Value *gep = generate_member_access(mae);
            return builder.CreateLoad(make_llvm_type(mae->type), gep, "deref_temp");
        }
        case PREFIX_EXPR: {
            auto pe = (PrefixOperatorExpression *) expression;

            if (pe->prefix_type == REF) {
                if (pe->operand->expression_type == NAME_EXPR) {
                    return variables.at(((NameExpression *) pe->operand)->name).first;
                } else if (pe->operand->expression_type == MEM_ACC_EXPR) {
                    return generate_member_access((BinaryOperatorExpression *) expression);
                }
            }

            llvm::Value *val = generate_expression(pe->operand);

            switch (pe->prefix_type) {
                case POS:
                    throw; // how would you do this in llvm?
                    break;
                case NEG:
                    if (val->getType()->isFloatingPointTy()) return builder.CreateFNeg(val, "neg_temp");
                    else return builder.CreateNeg(val, "neg_temp");
                case DEREF:
                    return builder.CreateLoad(make_llvm_type(pe->type), val, "deref_temp");
                case LOG_NOT:
                    return builder.CreateNot(val, "not_temp");
                    break;
                case REF:
                    break;
            }
            break;
        }
        case ASSIGN_EXPR: {
            auto ae = (BinaryOperatorExpression *) expression;
            llvm::Value *dest;
            llvm::Type *dest_type;
            if (ae->left->expression_type == NAME_EXPR) {
                auto var_on_stack = variables.at(((NameExpression *) ae->left)->name);
                dest = var_on_stack.first;
                dest_type = var_on_stack.second;
            } else if (ae->left->expression_type == MEM_ACC_EXPR) {
                dest = generate_member_access((BinaryOperatorExpression *) ae->left);
                dest_type = make_llvm_type(ae->left->type);
            } else if (ae->left->expression_type == PREFIX_EXPR && ((PrefixOperatorExpression *) ae->left)->prefix_type == DEREF) {
                auto deref = (PrefixOperatorExpression *) ae->left;
                dest = generate_expression(deref->operand);
                dest_type = make_llvm_type(deref->type);
            } else {
                dest = generate_expression(ae->left);
                dest_type = dest->getType();
            }
            return builder.CreateStore(generate_cast(generate_expression(ae->right), dest_type), dest, "assign_temp");
        }
        case NAME_EXPR: {
            auto ne = (NameExpression *) expression;
            auto var_on_stack = variables.at(ne->name);
            return builder.CreateLoad(var_on_stack.second, var_on_stack.first, "load_temp");
        }
        case INT_EXPR: {
            auto ie = (IntExpression *) expression;
            size_t width = std::max(8, roundUp(std::bit_width((size_t) ie->n), size_t(8)));
            return llvm::ConstantInt::get(llvm::Type::getIntNTy(context, width), ie->n, true);
        }
        case REAL_EXPR: {
            auto re = (RealExpression *) expression;
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), re->n);
        }
        case STR_EXPR: {
            auto se = (StringExpression *) expression;
            return builder.CreateGlobalStringPtr(se->n, "string_value");
        }
        case BOOL_EXPR: {
            auto be = (BoolExpression *) expression;
            return llvm::ConstantInt::get(llvm::Type::getIntNTy(context, 1), be->n, false);
        }
        case NULL_EXPR:
            return llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
    }
    return nullptr;
}

llvm::Value *LLVM::generate_cast(llvm::Value *val, llvm::Type *type, bool signed_int) {
    if (val->getType() == type) return val;
    llvm::Instruction::CastOps co;
    if (type->isFloatingPointTy()) {
        if (val->getType()->isFloatingPointTy()) {
            return builder.CreateFPCast(val, type, "cast_temp");
        } else {
            if (signed_int) co = llvm::Instruction::SIToFP;
            else co = llvm::Instruction::UIToFP;
        }
    } else {
        if (val->getType()->isFloatingPointTy()) {
            if (signed_int) co = llvm::Instruction::FPToSI;
            else co = llvm::Instruction::FPToUI;
        } else {
            return builder.CreateIntCast(val, type, signed_int, "cast_temp");
        }
    }
    return builder.CreateCast(co, val, type, "cast_temp");
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
            case BOOL:
                res = llvm::Type::getInt1Ty(context);
                break;
            case VOID:
                res = llvm::Type::getVoidTy(context);
        }
    } else {
        res = structures.at(t.type.user_type);
    }

    for (int i = 0; i < t.pointer_level; i++) {
        res = res->getPointerTo();
    }

    return res;
}

llvm::FunctionType *LLVM::make_llvm_function_type(FuncStCommon *func) {
    llvm::Type *rt = make_llvm_type(func->return_type);
    std::vector<llvm::Type *> argument_types;
    for (auto arg: func->arguments) {
        argument_types.push_back(make_llvm_type(arg->type));
    }
    llvm::FunctionType *func_type = llvm::FunctionType::get(rt, argument_types, func->var_arg);
    return func_type;
}

llvm::Value *LLVM::generate_member_access(BinaryOperatorExpression *mae) {
    std::string member_name = ((NameExpression *) mae->right)->name;

    llvm::Value *left;
    StructStatement *struct_;

    if (mae->left->expression_type == NAME_EXPR) {
        std::string var_name = ((NameExpression *) mae->left)->name;
        auto var = variables.at(var_name);

        auto it = std::find_if(structures.begin(), structures.end(), [var](auto pair) { return pair.second == var.second; });
        if (it == structures.end()) {/*jaix*/throw; }

        left = var.first;
        struct_ = it->first;
    } else {
        left = generate_expression(mae->left);
        struct_ = mae->left->type.type.user_type;
    }
    llvm::Type *struct_type = structures.at(struct_);

    int member_index = struct_->get_member_index(member_name);

    return builder.CreateStructGEP(struct_type, left, member_index, "member_load_temp");
}

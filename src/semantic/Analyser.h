// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_ANALYSER_H_
#define TARIK_SRC_SEMANTIC_ANALYSER_H_

#include "syntactic/expressions/Statements.h"

#include <vector>

#include "Variables.h"
#include "error/Bucket.h"

class Analyser {
    std::map<std::vector<std::string>, FuncStatement *> functions;
    std::map<std::vector<std::string>, StructStatement *> structures;
    std::map<std::vector<std::string>, FuncDeclareStatement *> declarations;

    std::vector<std::string> path;

    std::vector<SemanticVariable *> variables;
    Statement *last_loop = nullptr;
    unsigned int level = 0;
    Type return_type = Type(VOID);

    Bucket *bucket;

    [[nodiscard]] std::vector<std::string> get_global_path(const std::string &name) const;
    [[nodiscard]] std::vector<std::string> get_global_path(const std::vector<std::string> &name) const;
    [[nodiscard]] std::string flatten_path(const std::string &name = "") const;
    [[nodiscard]] static std::string flatten_path(const std::vector<std::string> &path);
    [[nodiscard]] static std::string flatten_path(std::vector<std::string> path, const std::string &name);

    struct __no_auto_main {};

    Analyser(Bucket *bucket, __no_auto_main _)
        : bucket(bucket) {}

public:
    Analyser(Bucket *bucket);

    std::vector<Statement *> finish();

    bool analyse(const std::vector<Statement *> &statements);

protected:
    bool analyse_import(const std::vector<Statement *> &statements);

    struct VariableVerificationResult {
        SemanticVariable *var;
        bool res;
    };

    bool verify_statement(Statement *statement);
    bool verify_statements(const std::vector<Statement *> &statements);
    bool verify_scope(ScopeStatement *scope, std::string name = "");
    bool verify_function(FuncStatement *func);
    bool verify_func_decl(FuncDeclareStatement *decl);
    bool verify_if(IfStatement *if_);
    bool verify_else(ElseStatement *else_);
    bool verify_return(ReturnStatement *return_);
    bool verify_while(WhileStatement *while_);
    bool verify_break(BreakStatement *break_);
    bool verify_continue(ContinueStatement *continue_);
    VariableVerificationResult verify_variable(VariableStatement *var);
    bool verify_struct(StructStatement *struct_);
    bool verify_import(ImportStatement *import_);
    bool verify_expression(Expression *expression, bool assigned_to = false, bool member_acc = false);
    bool verify_type(Type *type);

    bool does_always_return(ScopeStatement *scope);
    bool is_var_declared(const std::string &name);
    bool is_func_declared(const std::vector<std::string> &name);
    bool is_struct_declared(const std::vector<std::string> &name);

    SemanticVariable *get_variable(const std::string &name);
    FuncStCommon *get_func_decl(const std::vector<std::string> &name);
    StructStatement *get_struct(const std::vector<std::string> &name);
};

#endif //TARIK_SRC_SEMANTIC_ANALYSER_H_

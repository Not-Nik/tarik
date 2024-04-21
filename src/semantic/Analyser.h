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
    std::map<std::vector<std::string>, ast::FuncStatement *> functions;
    std::map<std::vector<std::string>, ast::StructStatement *> structures;
    std::map<std::vector<std::string>, ast::FuncDeclareStatement *> declarations;

    std::vector<std::string> path;

    std::vector<SemanticVariable *> variables;
    ast::Statement *last_loop = nullptr;
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

    std::vector<ast::Statement *> finish();

    bool analyse(const std::vector<ast::Statement *> &statements);

protected:
    bool analyse_import(const std::vector<ast::Statement *> &statements);

    struct VariableVerificationResult {
        SemanticVariable *var;
        bool res;
    };

    bool verify_statement(ast::Statement *statement);
    bool verify_statements(const std::vector<ast::Statement *> &statements);
    bool verify_scope(ast::ScopeStatement *scope, std::string name = "");
    bool verify_function(ast::FuncStatement *func);
    bool verify_func_decl(ast::FuncDeclareStatement *decl);
    bool verify_if(ast::IfStatement *if_);
    bool verify_else(ast::ElseStatement *else_);
    bool verify_return(ast::ReturnStatement *return_);
    bool verify_while(ast::WhileStatement *while_);
    bool verify_break(ast::BreakStatement *break_);
    bool verify_continue(ast::ContinueStatement *continue_);
    VariableVerificationResult verify_variable(ast::VariableStatement *var);
    bool verify_struct(ast::StructStatement *struct_);
    bool verify_import(ast::ImportStatement *import_);
    bool verify_expression(ast::Expression *expression, bool assigned_to = false, bool member_acc = false);
    bool verify_type(Type *type);

    bool does_always_return(ast::ScopeStatement *scope);
    bool is_var_declared(const std::string &name);
    bool is_func_declared(const std::vector<std::string> &name);
    bool is_struct_declared(const std::vector<std::string> &name);

    SemanticVariable *get_variable(const std::string &name);
    ast::FuncStCommon *get_func_decl(const std::vector<std::string> &name);
    ast::StructStatement *get_struct(const std::vector<std::string> &name);
};

#endif //TARIK_SRC_SEMANTIC_ANALYSER_H_

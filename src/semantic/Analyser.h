// tarik (c) Nikolas Wipper 2021-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_ANALYSER_H_
#define TARIK_SRC_SEMANTIC_ANALYSER_H_

#include "syntactic/expressions/Statements.h"

#include <vector>

class Analyser {
    std::map<std::vector<std::string>, FuncStatement *> functions;
    std::map<std::vector<std::string>, StructStatement *> structures;
    std::map<std::vector<std::string>, FuncDeclareStatement *> declarations;

    std::vector<std::string> path;

    std::vector<VariableStatement *> variables;
    Statement *last_loop = nullptr;
    unsigned int level = 0;

    [[nodiscard]] std::vector<std::string> get_local_path(const std::string& name) const;
    [[nodiscard]] std::string flatten_path(const std::string& name = "") const;
    [[nodiscard]] static std::string flatten_path(const std::vector<std::string>& path);
    [[nodiscard]] static std::string flatten_path(const std::vector<std::string>& path, const std::string& name);
public:
    Analyser();

    bool verify_statement(Statement *statement);
    bool verify_statements(const std::vector<Statement *> &statements);

    std::vector<Statement *> finish();

protected:
    bool verify_scope(ScopeStatement *scope, std::string name = "");
    bool verify_function(FuncStatement *func);
    bool verify_func_decl(FuncDeclareStatement *decl);
    bool verify_if(IfStatement *if_);
    bool verify_else(ElseStatement *else_);
    bool verify_return(ReturnStatement *return_);
    bool verify_while(WhileStatement *while_);
    bool verify_break(BreakStatement *break_);
    bool verify_continue(ContinueStatement *continue_);
    bool verify_variable(VariableStatement *var);
    bool verify_struct(StructStatement *struct_);
    bool verify_import(ImportStatement *import_);
    bool verify_expression(Expression *expression);

    bool does_always_return(ScopeStatement *scope);
    bool is_var_declared(const std::string &name);
    bool is_func_declared(const std::vector<std::string> &name);
    bool is_struct_declared(const std::vector<std::string> &name);

    VariableStatement *get_variable(const std::string &name);
    FuncStCommon *get_func_decl(const std::vector<std::string> &name);
    StructStatement *get_struct(const std::vector<std::string> &name);
};

#endif //TARIK_SRC_SEMANTIC_ANALYSER_H_

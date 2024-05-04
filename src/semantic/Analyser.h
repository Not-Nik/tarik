// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_ANALYSER_H_
#define TARIK_SRC_SEMANTIC_ANALYSER_H_

#include <vector>

#include "Macro.h"
#include "error/Bucket.h"
#include "semantic/ast/Statements.h"
#include "syntactic/ast/Statements.h"
#include "Variables.h"

class Analyser {
    std::vector<aast::FuncStatement *> functions;
    std::unordered_map<Path, aast::StructStatement *> structures;
    std::unordered_map<Path, aast::FuncDeclareStatement *> declarations;

    std::unordered_map<std::string, Macro *> macros;

    Path path = Path({});

    std::vector<SemanticVariable *> variables;
    ast::Statement *last_loop = nullptr;
    unsigned int level = 0;
    Type return_type = Type(VOID);

    Bucket *bucket;

public:
    Analyser(Bucket *bucket);

    std::vector<aast::Statement *> finish();

    void analyse(const std::vector<ast::Statement *> &statements);

protected:
    enum AccessType {
        NORMAL,
        ASSIGNMENT,
        MOVE
    };

    void analyse_import(const std::vector<ast::Statement *> &statements);

    std::optional<aast::Statement *> verify_statement(ast::Statement *statement);
    std::optional<std::vector<aast::Statement *>> verify_statements(const std::vector<ast::Statement *> &statements);
    std::optional<aast::ScopeStatement *> verify_scope(ast::ScopeStatement *scope, std::string name = "");
    std::optional<aast::FuncStatement *> verify_function(ast::FuncStatement *func);
    std::optional<aast::IfStatement *> verify_if(ast::IfStatement *if_);
    std::optional<aast::ElseStatement *> verify_else(ast::ElseStatement *else_);
    std::optional<aast::ReturnStatement *> verify_return(ast::ReturnStatement *return_);
    std::optional<aast::WhileStatement *> verify_while(ast::WhileStatement *while_);
    std::optional<aast::BreakStatement *> verify_break(ast::BreakStatement *break_);
    std::optional<aast::ContinueStatement *> verify_continue(ast::ContinueStatement *continue_);
    std::optional<SemanticVariable *> verify_variable(ast::VariableStatement *var);
    std::optional<aast::StructStatement *> verify_struct(ast::StructStatement *struct_);
    std::optional<aast::ImportStatement *> verify_import(ast::ImportStatement *import_);
    std::optional<aast::Expression *> verify_expression(ast::Expression *expression,
                                                        AccessType access = NORMAL,
                                                        bool member_acc = false);
    std::optional<Type> verify_type(Type type);

    bool does_always_return(ast::ScopeStatement *scope);
    bool is_var_declared(const std::string &name);
    bool is_func_declared(Path path);
    bool is_struct_declared(Path path);

    SemanticVariable *get_variable(const std::string &name);
    aast::FuncDeclareStatement *get_func_decl(Path path);
    aast::StructStatement *get_struct(Path path);
};

#endif //TARIK_SRC_SEMANTIC_ANALYSER_H_

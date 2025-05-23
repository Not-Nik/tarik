// tarik (c) Nikolas Wipper 2021-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_ANALYSER_H_
#define TARIK_SRC_SEMANTIC_ANALYSER_H_

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Macro.h"
#include "Structures.h"
#include "error/Bucket.h"
#include "semantic/ast/Statements.h"
#include "syntactic/ast/Statements.h"
#include "Variables.h"

namespace lifetime
{
class Analyser;
}

class Analyser {
    friend class lifetime::Analyser;

    std::vector<aast::FuncStatement *> functions;
    std::unordered_map<Path, aast::StructStatement *> structures;

    std::unordered_map<Path, aast::StructDeclareStatement *> struct_decls;
    std::unordered_map<Path, aast::FuncDeclareStatement *> func_decls;

    std::unordered_map<std::string, Macro *> macros;

    Path path = Path({});

    std::vector<SemanticVariable *> variables;
    std::unordered_map<std::string, std::string> variable_names;
    std::unordered_set<std::string> used_names;

    ast::Statement *last_loop = nullptr;
    unsigned int level = 0;
    Type return_type = Type(VOID);

    Bucket *bucket;
    StructureGraph structure_graph;

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
    void verify_structs(const std::vector<ast::Statement *> &statements);

    std::optional<aast::Statement *> verify_statement(ast::Statement *statement);
    std::optional<std::vector<aast::Statement *>> verify_statements(const std::vector<ast::Statement *> &statements);
    std::optional<aast::ScopeStatement *> verify_scope(ast::ScopeStatement *scope, const std::string &name = "");
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

    std::optional<aast::Expression *> verify_call_expression(ast::Expression *expression,
                                                             AccessType access = NORMAL,
                                                             bool member_acc = false);
    std::optional<aast::Expression *> verify_macro_expression(ast::Expression *expression);
    std::optional<aast::BinaryExpression *> verify_binary_expression(ast::Expression *expression,
                                                                     AccessType access = NORMAL);
    std::optional<aast::BinaryExpression *> verify_member_access_expression(ast::Expression *expression,
                                                                            AccessType access = NORMAL);
    std::optional<aast::PrefixExpression *> verify_prefix_expression(ast::Expression *expression);
    std::optional<aast::NameExpression *> verify_name_expression(ast::Expression *expression,
                                                                 AccessType access = NORMAL,
                                                                 bool member_acc = false);

    std::optional<Type> verify_type(Type type);
    std::string get_unused_var_name(const std::string &candidate);

    bool does_always_return(ast::ScopeStatement *scope);
    bool is_var_declared(std::string name) const;
    bool is_func_declared(const Path &path) const;
    bool is_struct_declared(const Path &path) const;

    SemanticVariable *get_variable(std::string name) const;
    aast::FuncDeclareStatement *get_func_decl(const Path &path) const;
    aast::StructStatement *get_struct(const Path &path) const;
    aast::StructDeclareStatement *get_struct_decl(const Path &path) const;
};

#endif //TARIK_SRC_SEMANTIC_ANALYSER_H_

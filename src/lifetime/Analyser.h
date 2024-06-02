// tarik (c) Nikolas Wipper 2024

#ifndef TARIK_SRC_LIFETIME_ANALYSER_H_
#define TARIK_SRC_LIFETIME_ANALYSER_H_

#include <map>

#include "error/Bucket.h"
#include "semantic/Analyser.h"
#include "semantic/ast/Statements.h"
#include "Variable.h"

namespace lifetime
{
struct Function {
    std::map<std::string, VariableState *> variables;
    std::vector<LexerRange> statement_positions;
    std::unordered_map<Lifetime*, std::vector<Lifetime*>> relations;
};

class Analyser {
    Bucket *bucket;
    std::unordered_map<Path, aast::StructStatement *> structures;
    std::unordered_map<Path, aast::FuncDeclareStatement *> declarations;

    std::unordered_map<Path, Function> functions;

    std::size_t statement_index = 0;
    Function *current_function = nullptr;
    std::vector<std::map<std::string, VariableState *>> variables;

public:
    Analyser(Bucket *bucket, ::Analyser *analyser);

    void analyse(const std::vector<aast::Statement *> &statements);

private:
    void analyse_statements(const std::vector<aast::Statement *> &statements);
    void analyse_statement(aast::Statement *statement);
    void analyse_scope(aast::ScopeStatement *scope, bool dont_init_vars = false);
    void analyse_function(aast::FuncStatement *func);
    void analyse_if(aast::IfStatement *if_);
    void analyse_return(aast::ReturnStatement *return_);
    void analyse_while(aast::WhileStatement *while_);
    VariableState *analyse_variable(aast::VariableStatement *var, bool argument = false);
    void analyse_import(aast::ImportStatement *import_);
    void analyse_expression(aast::Expression *expression, int depth = 0);

    void verify_statements(const std::vector<aast::Statement *> &statements);
    void verify_statement(aast::Statement *statement);
    void verify_scope(aast::ScopeStatement *scope);
    void verify_function(aast::FuncStatement *func);
    void verify_if(aast::IfStatement *if_);
    void verify_return(aast::ReturnStatement *return_);
    void verify_while(aast::WhileStatement *while_);
    void verify_import(aast::ImportStatement *import_);
    Lifetime *verify_expression(aast::Expression *expression, bool assigned = false);

    VariableState *get_variable(std::string name);

    bool is_within(Lifetime *a, Lifetime *b, bool rec = false) const;
    void print_lifetime_error(const aast::Expression *left,
                              const aast::Expression *right,
                              Lifetime *a,
                              Lifetime *b,
                              bool rec =
                                      false) const;

    bool is_shorter(Lifetime *a, Lifetime *b) const;
};
} // lifetime

#endif //TARIK_SRC_LIFETIME_ANALYSER_H_

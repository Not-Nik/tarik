// tarik (c) Nikolas Wipper 2024

#ifndef TARIK_SRC_LIFETIME_ANALYSER_H_
#define TARIK_SRC_LIFETIME_ANALYSER_H_

#include <map>

#include "error/Bucket.h"
#include "semantic/Analyser.h"
#include "semantic/ast/Statements.h"

namespace lifetime
{
struct Lifetime {
    std::size_t birth, death, last_death;

    Lifetime(std::size_t at);
};

struct Variable {
    std::vector<Lifetime> lifetimes;

    Variable();

    void used(std::size_t at);
    void rebirth(std::size_t at); // oh how biblical
    void add(Lifetime lifetime);
};

struct Function {
    std::map<std::string, Variable> lifetimes;
};

class Analyser {
    Bucket *bucket;
    std::unordered_map<Path, aast::StructStatement *> structures;
    std::unordered_map<Path, aast::FuncDeclareStatement *> declarations;

    std::unordered_map<Path, Function> functions;

    std::size_t statement_index = 0;
    Function *current_function = nullptr;
    std::vector<std::map<std::string, Variable>> variables;

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
    void analyse_variable(aast::VariableStatement *var, bool argument = false);
    void analyse_import(aast::ImportStatement *import_);
    void analyse_expression(aast::Expression *expression);

    Variable &get_variable(std::string name);
};
} // lifetime

#endif //TARIK_SRC_LIFETIME_ANALYSER_H_

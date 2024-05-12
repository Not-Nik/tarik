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
    bool temp = false;

    Lifetime(std::size_t at);
    static Lifetime static_();
    static Lifetime temporary(std::size_t at);
private:
    Lifetime(std::size_t birth, std::size_t deaths);
};

struct Variable {
    Lifetime lifetime;
    std::vector<Lifetime> values;

    Variable(std::size_t at);

    void used(std::size_t at);
    void assigned(std::size_t at);

    void kill(std::size_t at);
    void move(std::size_t at);

    Lifetime current(std::size_t at);
    Lifetime current_continuous(std::size_t at);
};

struct Function {
    std::map<std::string, Variable> variables;
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

    void verify_statements(const std::vector<aast::Statement *> &statements);
    void verify_statement(aast::Statement *statement);
    void verify_scope(aast::ScopeStatement *scope);
    void verify_function(aast::FuncStatement *func);
    void verify_if(aast::IfStatement *if_);
    void verify_return(aast::ReturnStatement *return_);
    void verify_while(aast::WhileStatement *while_);
    void verify_import(aast::ImportStatement *import_);
    Lifetime verify_expression(aast::Expression *expression, bool assigned = false);

    Variable &get_variable(std::string name);
};
} // lifetime

#endif //TARIK_SRC_LIFETIME_ANALYSER_H_

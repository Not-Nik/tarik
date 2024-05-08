// tarik (c) Nikolas Wipper 2024

#include "Analyser.h"

#include <iostream>
#include <utility>

namespace lifetime
{
Lifetime::Lifetime(std::size_t at)
    : birth(at),
      death(at),
      last_death(at) {}

Variable::Variable()
    : lifetimes({}) {}

void Variable::used(std::size_t at) {
    if (lifetimes.empty()) {
        lifetimes.emplace_back(at);
    }
    lifetimes.back().death = at;
    lifetimes.back().last_death = at;
}

void Variable::rebirth(std::size_t at) {
    if (!lifetimes.empty()) {
        used(at);
    }
    lifetimes.emplace_back(at);
}

void Variable::add(Lifetime lifetime) {
    lifetimes.push_back(lifetime);
}

Analyser::Analyser(Bucket *bucket, ::Analyser *analyser)
    : bucket(bucket),
      structures(analyser->structures),
      declarations(analyser->declarations) {}

void Analyser::analyse(const std::vector<aast::Statement *> &statements) {
    analyse_statements(statements);
}

void Analyser::analyse_statements(const std::vector<aast::Statement *> &statements) {
    for (auto *statement : statements) {
        analyse_statement(statement);
    }
}

void Analyser::analyse_statement(aast::Statement *statement) {
    switch (statement->statement_type) {
        case aast::SCOPE_STMT:
            analyse_scope((aast::ScopeStatement *) statement);
            break;
        case aast::FUNC_STMT:
            analyse_function((aast::FuncStatement *) statement);
            break;
        case aast::IF_STMT:
            analyse_if((aast::IfStatement *) statement);
            break;
        case aast::RETURN_STMT:
            analyse_return((aast::ReturnStatement *) statement);
            break;
        case aast::WHILE_STMT:
            analyse_while((aast::WhileStatement *) statement);
            break;
            break;
        case aast::VARIABLE_STMT:
            analyse_variable((aast::VariableStatement *) statement);
            break;
        case aast::IMPORT_STMT:
            analyse_import((aast::ImportStatement *) statement);
            break;
        case aast::EXPR_STMT:
            analyse_expression((aast::Expression *) statement);
            break;
        case aast::ELSE_STMT:
        case aast::BREAK_STMT:
        case aast::CONTINUE_STMT:
        case aast::STRUCT_STMT:
        default:
            break;
    }
    statement_index++;
}

void Analyser::analyse_scope(aast::ScopeStatement *scope, bool dont_init_vars) {
    if (!dont_init_vars)
        variables.emplace_back();

    analyse_statements(scope->block);

    std::map<std::string, Variable> current_scope = variables.back();
    variables.pop_back();

    // Go through all the variables created in this scope
    for (auto [name, var] : current_scope) {
        // If it ever lived
        if (!var.lifetimes.empty())
            // Mark the current location as the last possible place it could die
            var.lifetimes.back().last_death = statement_index;
        // If we already had a cariable of the same name in a previous scope (mind you this does not mean a scope
        // above the current on, but literally before it in the file, i.e. a previous line)
        if (current_function->lifetimes.contains(name)) {
            // Add all the lifetimes to the previous ones
            for (auto lifetime : var.lifetimes)
                current_function->lifetimes.at(name).add(lifetime);
        } else {
            // Otherwise just emplace it
            current_function->lifetimes.emplace(name, var);
        }
    }
}

void Analyser::analyse_function(aast::FuncStatement *func) {
    functions.emplace(func->path, Function {});

    statement_index = 0;
    current_function = &functions.at(func->path);

    variables.emplace_back();

    for (auto *argument : func->arguments) {
        analyse_variable(argument, true);
    }
    statement_index++;

    analyse_scope(func, true);
}

void Analyser::analyse_if(aast::IfStatement *if_) {
    analyse_expression(if_->condition);
    analyse_scope(if_);
    if (if_->else_statement) {
        analyse_scope(if_->else_statement);
    }
}

void Analyser::analyse_return(aast::ReturnStatement *return_) {
    if (return_->value)
        analyse_expression(return_->value);
}

void Analyser::analyse_while(aast::WhileStatement *while_) {
    analyse_expression(while_->condition);
    analyse_scope(while_);
}

void Analyser::analyse_import(aast::ImportStatement *import_) {
    analyse_scope(import_);
}

void Analyser::analyse_variable(aast::VariableStatement *var, bool argument) {
    variables.back().emplace(var->name.raw, Variable());
    if (argument)
        variables.back().at(var->name.raw).rebirth(0);
}

void Analyser::analyse_expression(aast::Expression *expression) {
    switch (expression->expression_type) {
        case aast::CALL_EXPR: {
            auto *ce = (aast::CallExpression *) expression;

            for (auto *argument : ce->arguments)
                analyse_expression(argument);
            break;
        }
        case aast::DASH_EXPR:
        case aast::DOT_EXPR:
        case aast::EQ_EXPR:
        case aast::COMP_EXPR: {
            auto *be = (aast::BinaryExpression *) expression;

            analyse_expression(be->left);
            analyse_expression(be->right);
            break;
        }
        case aast::MEM_ACC_EXPR: {
            auto *mae = (aast::BinaryExpression *) expression;

            // We don't care about which member is accessed, if a member is access the entire struct has to live to that
            // point, because structs always have the exact same lifetime as their members
            analyse_expression(mae->left);
            break;
        }
        case aast::PREFIX_EXPR: {
            auto *pe = (aast::PrefixExpression *) expression;
            analyse_expression(pe->operand);
            break;
        }
        case aast::ASSIGN_EXPR: {
            auto *ae = (aast::BinaryExpression *) expression;

            aast::Expression *left = ae->left;
            bool mem_acc = left->expression_type == aast::MEM_ACC_EXPR;

            // Again, we don't care about which member is assigned to
            while (left->expression_type == aast::MEM_ACC_EXPR) {
                left = ((aast::BinaryExpression *) left)->left;
            }

            // But if a member is assigned, that doesn't creat a new lifetime for the struct
            // fixme: This could potentially create cases where this analysis detects an error in a piece of code, even
            //  though it is actually valid.
            // todo: Solve this by tracking individual member states like we do in semantic analysis.
            if (left->expression_type == aast::NAME_EXPR && !mem_acc) {
                std::string var_name = left->print();
                get_variable(var_name).rebirth(statement_index);
            } else {
                // In normal expressions, variables don't create a new lifetime
                analyse_expression(left);
            }
            analyse_expression(ae->right);
            break;
        }
        case aast::NAME_EXPR: {
            auto *ne = (aast::NameExpression *) expression;
            get_variable(ne->name).used(statement_index);
            break;
        }
        case aast::INT_EXPR:
        case aast::BOOL_EXPR:
        case aast::REAL_EXPR:
        case aast::STR_EXPR:
            // todo: give strings static lifetime
            break;
        case aast::CAST_EXPR: {
            auto *ce = (aast::CastExpression *) expression;
            analyse_expression(ce->expression);
            break;
        }
    }
}

Variable &Analyser::get_variable(std::string name) {
    for (auto &scope : variables) {
        if (scope.contains(name))
            return scope.at(name);
    }
    std::unreachable();
}
} // lifetime

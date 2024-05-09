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

Lifetime::Lifetime(std::size_t birth, std::size_t deaths)
    : birth(birth),
      death(deaths),
      last_death(deaths) {}


Lifetime Lifetime::static_() {
    return {0, std::numeric_limits<std::size_t>::max()};
}

Lifetime Lifetime::temporary(std::size_t at) {
    Lifetime t = {at};
    t.temp = true;
    return t;
}

Variable::Variable(std::size_t at)
    : lifetime(at),
      values({}) {}

void Variable::used(std::size_t at) {
    if (values.empty()) {
        values.emplace_back(at);
    }
    values.back().death = std::max(values.back().death, at);
    values.back().last_death = std::max(values.back().last_death, at);

    lifetime.death = at;
}

void Variable::assigned(std::size_t at) {
    if (!values.empty())
        values.back().last_death = at;
    values.emplace_back(at);

    lifetime.death = at;
}

void Variable::kill(std::size_t at) {
    lifetime.last_death = std::max(lifetime.last_death, at);
}

Lifetime Variable::current(std::size_t at) {
    for (auto &lifetime : values) {
        if (lifetime.birth <= at) {
            if (lifetime.last_death >= at)
                return lifetime;
        }
    }
    std::unreachable();
}

Analyser::Analyser(Bucket *bucket, ::Analyser *analyser)
    : bucket(bucket),
      structures(analyser->structures),
      declarations(analyser->declarations) {}

void Analyser::analyse(const std::vector<aast::Statement *> &statements) {
    analyse_statements(statements);
    verify_statements(statements);
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
        // Mark the current location as the last possible place it could die
        var.kill(statement_index);
        // Otherwise just emplace it
        current_function->variables.emplace(name, var);
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

    std::cout << "In " << func->path.str() << std::endl;
    for (auto [name, var] : current_function->variables) {
        std::cout << "\t" << name << " (" << var.lifetime.birth << "-" << var.lifetime.death << " (" << var.lifetime.
                last_death << ")" << ":" << std::endl;
        for (auto lifetime : var.values) {
            std::cout << "\t\t" << lifetime.birth << "-" << lifetime.death << " (" << lifetime.last_death << ")" <<
                    std::endl;
        }
    }
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
    variables.back().emplace(var->name.raw, Variable(statement_index));
    if (argument) {
        Variable &arg = variables.back().at(var->name.raw);
        arg.assigned(0);
        if (var->type.pointer_level > 0)
            // To the function, a pointer given as an argument is as static as it gets
            arg.lifetime = Lifetime::static_();
    }
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
                get_variable(var_name).assigned(statement_index);
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

void Analyser::verify_statements(const std::vector<aast::Statement *> &statements) {
    for (auto *statement : statements) {
        verify_statement(statement);
    }
}

void Analyser::verify_statement(aast::Statement *statement) {
    switch (statement->statement_type) {
        case aast::SCOPE_STMT:
            verify_scope((aast::ScopeStatement *) statement);
            break;
        case aast::FUNC_STMT:
            verify_function((aast::FuncStatement *) statement);
            break;
        case aast::IF_STMT:
            verify_if((aast::IfStatement *) statement);
            break;
        case aast::RETURN_STMT:
            verify_return((aast::ReturnStatement *) statement);
            break;
        case aast::WHILE_STMT:
            verify_while((aast::WhileStatement *) statement);
            break;
        case aast::IMPORT_STMT:
            verify_import((aast::ImportStatement *) statement);
            break;
        case aast::EXPR_STMT:
            verify_expression((aast::Expression *) statement);
            break;
        case aast::ELSE_STMT:
        case aast::BREAK_STMT:
        case aast::CONTINUE_STMT:
        case aast::VARIABLE_STMT:
        case aast::STRUCT_STMT:
        default:
            break;
    }
    statement_index++;
}

void Analyser::verify_scope(aast::ScopeStatement *scope) {
    verify_statements(scope->block);
}

void Analyser::verify_function(aast::FuncStatement *func) {
    statement_index = 1;
    current_function = &functions.at(func->path);

    verify_scope(func);
}

void Analyser::verify_if(aast::IfStatement *if_) {
    verify_expression(if_->condition);
    verify_scope(if_);
    if (if_->else_statement) {
        verify_scope(if_->else_statement);
    }
}

void Analyser::verify_return(aast::ReturnStatement *return_) {
    if (return_->value)
        verify_expression(return_->value);
}

void Analyser::verify_while(aast::WhileStatement *while_) {
    verify_expression(while_->condition);
    verify_scope(while_);
}

void Analyser::verify_import(aast::ImportStatement *import_) {
    verify_scope(import_);
}

Lifetime Analyser::verify_expression(aast::Expression *expression, bool assigned) {
    switch (expression->expression_type) {
        case aast::CALL_EXPR: {
            auto *ce = (aast::CallExpression *) expression;

            for (auto *argument : ce->arguments)
                verify_expression(argument);
            // Todo: check for pointers
            return Lifetime::temporary(statement_index);
        }
        case aast::DASH_EXPR:
        case aast::DOT_EXPR:
        case aast::EQ_EXPR:
        case aast::COMP_EXPR: {
            auto *be = (aast::BinaryExpression *) expression;

            verify_expression(be->left);
            verify_expression(be->right);
            return Lifetime::temporary(statement_index);
        }
        case aast::MEM_ACC_EXPR: {
            auto *mae = (aast::BinaryExpression *) expression;

            // We don't care about which member is accessed, if a member is access the entire struct has to live to that
            // point, because structs always have the exact same lifetime as their members
            return verify_expression(mae->left);
        }
        case aast::PREFIX_EXPR: {
            auto *pe = (aast::PrefixExpression *) expression;
            Lifetime lifetime = verify_expression(pe->operand);
            if (pe->prefix_type == aast::REF) {
                if (lifetime.temp)
                    return {statement_index};
                return lifetime;
            } else if (assigned && pe->prefix_type == aast::DEREF) {
                return lifetime;
            }
            return Lifetime::temporary(statement_index);
        }
        case aast::ASSIGN_EXPR: {
            auto *ae = (aast::BinaryExpression *) expression;

            Lifetime right_lifetime = verify_expression(ae->right);
            Lifetime left_lifetime = verify_expression(ae->left, true);

            bucket->iassert(right_lifetime.temp || left_lifetime.death <= right_lifetime.last_death,
                            ae->right->origin,
                            "value does not live long enough");
            // todo: translate statement index to origin and print where we think the value dies

            return Lifetime::static_();
        }
        case aast::NAME_EXPR: {
            auto *ne = (aast::NameExpression *) expression;
            if (assigned)
                return current_function->variables.at(ne->name).current(statement_index);
            return current_function->variables.at(ne->name).lifetime;
        }
        case aast::INT_EXPR:
        case aast::BOOL_EXPR:
        case aast::REAL_EXPR:
            return Lifetime::temporary(statement_index);
        case aast::STR_EXPR:
            return Lifetime::static_();
        case aast::CAST_EXPR: {
            auto *ce = (aast::CastExpression *) expression;
            Lifetime l = verify_expression(ce->expression);
            l.temp = true;
            return l;
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

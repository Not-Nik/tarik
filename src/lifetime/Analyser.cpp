// tarik (c) Nikolas Wipper 2024

#include "Analyser.h"

#include "Variable.h"

#include <iostream>
#include <utility>

namespace lifetime
{
Lifetime::Lifetime(std::size_t at)
    : birth(at),
      death(at),
      last_death(0) {}

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

bool Lifetime::operator==(const Lifetime &o) const {
    return birth == o.birth && death == o.death && last_death == o.last_death;
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

    std::map<std::string, VariableState *> current_scope = variables.back();
    variables.pop_back();

    // Go through all the variables created in this scope
    for (auto [name, var] : current_scope) {
        // Mark the current location as the last possible place it could die
        var->kill(statement_index);
        // Otherwise just emplace it
        current_function->variables.emplace(name, var);
    }
}

void Analyser::analyse_function(aast::FuncStatement *func) {
    functions.emplace(func->path, Function {});

    statement_index = 0;
    current_function = &functions.at(func->path);

    variables.emplace_back();

    std::vector<VariableState *> argument_states;
    argument_states.reserve(func->arguments.size());
    for (auto *argument : func->arguments) {
        argument_states.push_back(analyse_variable(argument, true));
    }
    statement_index++;

    analyse_scope(func, true);

    for (auto *state : argument_states) {
        if (state->values.size() == 1 && state->lifetime == Lifetime::static_())
            state->values[0] = Lifetime::static_();
    }

    std::cout << "In " << func->path.str() << std::endl;
    for (const auto &[name, var] : current_function->variables) {
        std::cout << "\t" << name << " " << var->lifetime.birth << "-" << var->lifetime.death << " (" << var->lifetime.
                last_death << ")" << ":" << std::endl;
        for (auto lifetime : var->values) {
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

VariableState *Analyser::analyse_variable(aast::VariableStatement *var, bool argument) {
    VariableState *state;

    if (var->type.is_primitive()) {
        state = new VariableState(statement_index);
    } else {
        aast::StructStatement *st = structures.at(var->type.get_user());

        std::vector<VariableState *> member_states;
        for (auto *member : st->members) {
            auto *temp = new aast::VariableStatement(var->origin,
                                                     member->type,
                                                     Token::name(var->name.raw + "." + member->name.raw));

            VariableState *semantic_member = analyse_variable(temp, argument);
            member_states.push_back(semantic_member);
        }

        state = new CompoundState(statement_index, member_states);
    }

    variables.back().emplace(var->name.raw, state);
    if (argument) {
        state->assigned(0);
        if (var->type.pointer_level > 0)
            // To the function, a pointer given as an argument is as static as it gets
            state->lifetime = Lifetime::static_();
    }

    return state;
}

void Analyser::analyse_expression(aast::Expression *expression) {
    switch (expression->expression_type) {
        case aast::CALL_EXPR: {
            auto *ce = (aast::CallExpression *) expression;

            for (auto *argument : ce->arguments) {
                analyse_expression(argument);
                if (argument->flattens_to_member_access() && !argument->type.is_copyable()) {
                    // Move if variable has non-copyable type
                    get_variable(argument->flatten_to_member_access())->move(statement_index);
                }
            }
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

            analyse_expression(mae->left);

            auto *name = new aast::NameExpression(mae->origin, mae->type, mae->flatten_to_member_access());
            analyse_expression(name);
            delete name;

            break;
        }
        case aast::PREFIX_EXPR: {
            auto *pe = (aast::PrefixExpression *) expression;
            analyse_expression(pe->operand);
            break;
        }
        case aast::ASSIGN_EXPR: {
            auto *ae = (aast::BinaryExpression *) expression;

            if (ae->left->flattens_to_member_access()) {
                get_variable(ae->left->flatten_to_member_access())->assigned(statement_index);
            } else {
                // In normal expressions, variables don't create a new lifetime
                analyse_expression(ae->left);
            }
            analyse_expression(ae->right);
            break;
        }
        case aast::NAME_EXPR: {
            auto *ne = (aast::NameExpression *) expression;
            get_variable(ne->name)->used(statement_index);
            break;
        }
        case aast::INT_EXPR:
        case aast::BOOL_EXPR:
        case aast::REAL_EXPR:
        case aast::STR_EXPR:
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

            verify_expression(mae->left);

            auto *name = new aast::NameExpression(mae->origin, mae->type, mae->flatten_to_member_access());
            Lifetime lt = verify_expression(name);
            delete name;

            return lt;
        }
        case aast::PREFIX_EXPR: {
            auto *pe = (aast::PrefixExpression *) expression;
            Lifetime lifetime = verify_expression(pe->operand, assigned);
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
                return current_function->variables.at(ne->name)->current(statement_index);
            // Get timeframe, where this variable has a value, i.e. can be accessed by a pointer
            return current_function->variables.at(ne->name)->current_continuous(statement_index);
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

VariableState *Analyser::get_variable(std::string name) {
    for (auto &scope : variables) {
        if (scope.contains(name))
            return scope.at(name);
    }
    std::unreachable();
}
} // lifetime

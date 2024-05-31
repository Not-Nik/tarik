// tarik (c) Nikolas Wipper 2024

#include "Analyser.h"

#include "Variable.h"

#include <iostream>
#include <utility>

namespace lifetime
{
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
    if (current_function)
        current_function->statement_positions.push_back(statement->origin);
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
        if (state->values.size() == 1 && state->lifetime == LocalLifetime::static_(nullptr))
            state->values[0] = LocalLifetime::static_(nullptr);
    }

    std::cout << "In " << func->path.str() << std::endl;
    for (const auto &[name, var] : current_function->variables) {
        std::cout << "\t" << name;
        Lifetime *target = var->lifetime;

        while (target) {
            if (target->is_local()) {
                auto *local = (LocalLifetime *) target;
                std::cout << " " << local->birth << "-" << local->death << " (" << local->last_death << ")";
                target = target->next;
            } else {
                break;
            }
        }

        std::cout << ":" << std::endl;

        for (auto lifetime : var->values) {
            std::cout << "\t\t" << lifetime->birth << "-" << lifetime->death << " (" << lifetime->last_death << ")" <<
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

    Lifetime *lt = nullptr;

    for (int i = 0; i < var->type.pointer_level; i++)
        // Todo: store that the previous lt lives longer than the new one
        lt = argument ? new Lifetime(lt) : new LocalLifetime(lt, statement_index);

    auto *llt = new LocalLifetime(lt, statement_index);

    if (var->type.is_primitive() || var->type.pointer_level > 0) {
        state = new VariableState(llt, statement_index);
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

        state = new CompoundState(llt, statement_index, member_states);
    }

    variables.back().emplace(var->name.raw, state);
    if (argument) {
        state->assigned(0);
    }

    return state;
}

void Analyser::analyse_expression(aast::Expression *expression, int depth) {
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

            if (mae->left->type.pointer_level == 0) {
                analyse_expression(mae->left);

                auto *name = new aast::NameExpression(mae->origin, mae->type, mae->flatten_to_member_access());
                analyse_expression(name);
                delete name;
            } else {
                // Ignore depth of previous derefs
                analyse_expression(mae->left, 1);
            }

            break;
        }
        case aast::PREFIX_EXPR: {
            auto *pe = (aast::PrefixExpression *) expression;
            if (pe->prefix_type == aast::DEREF) {
                analyse_expression(pe->operand, depth + 1);
            } else {
                analyse_expression(pe->operand);
            }
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
            get_variable(ne->name)->used(statement_index, depth);
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

Lifetime *Analyser::verify_expression(aast::Expression *expression, bool assigned) {
    switch (expression->expression_type) {
        case aast::CALL_EXPR: {
            auto *ce = (aast::CallExpression *) expression;

            for (auto *argument : ce->arguments)
                verify_expression(argument);
            // Todo: check for pointers
            return LocalLifetime::temporary(nullptr, statement_index);
        }
        case aast::DASH_EXPR:
        case aast::DOT_EXPR:
        case aast::EQ_EXPR:
        case aast::COMP_EXPR: {
            auto *be = (aast::BinaryExpression *) expression;

            verify_expression(be->left);
            verify_expression(be->right);
            return LocalLifetime::temporary(nullptr, statement_index);
        }
        case aast::MEM_ACC_EXPR: {
            auto *mae = (aast::BinaryExpression *) expression;

            Lifetime *lt = verify_expression(mae->left);

            if (mae->left->type.pointer_level == 0) {
                auto *name = new aast::NameExpression(mae->origin, mae->type, mae->flatten_to_member_access());
                lt = verify_expression(name);
                delete name;
            }

            return lt;
        }
        case aast::PREFIX_EXPR: {
            auto *pe = (aast::PrefixExpression *) expression;
            Lifetime *lifetime = verify_expression(pe->operand, assigned);
            if (pe->prefix_type == aast::REF) {
                return LocalLifetime::temporary(lifetime, statement_index);
            } else if (assigned && pe->prefix_type == aast::DEREF) {
                return lifetime->next;
            }
            return LocalLifetime::temporary(lifetime->next, statement_index);
        }
        case aast::ASSIGN_EXPR: {
            auto *ae = (aast::BinaryExpression *) expression;

            Lifetime *right_lifetime = verify_expression(ae->right);
            Lifetime *left_lifetime = verify_expression(ae->left, true);

            if (!bucket->iassert(is_within(left_lifetime, right_lifetime),
                                 ae->right->origin,
                                 "value does not live long enough")) {
                print_lifetime_error(ae->left, ae->right, left_lifetime, right_lifetime);
            }
            // todo: translate statement index to origin and print where we think the value dies

            return LocalLifetime::static_(nullptr);
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
            return LocalLifetime::temporary(nullptr, statement_index);
        case aast::STR_EXPR:
            return LocalLifetime::temporary(LocalLifetime::static_(nullptr), statement_index);
        case aast::CAST_EXPR: {
            auto *ce = (aast::CastExpression *) expression;
            Lifetime *lt = verify_expression(ce->expression);
            return LocalLifetime::temporary(lt->next, statement_index);
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

bool Analyser::is_within(Lifetime *a, Lifetime *b, bool rec) const {
    if (a->is_local() && !b->is_local())
        return true;

    if (!a->is_local() && b->is_local())
        return false;

    if (a->is_local()) {
        auto *local_a = (LocalLifetime *) a;
        auto *local_b = (LocalLifetime *) b;

        if (rec && local_b->is_temp()) {
            return false;
        } else if (local_a->death <= local_b->last_death) {
            // todo: properly order deaths
            local_a->last_death = std::min(local_a->last_death, local_b->last_death);
        } else if (!local_b->is_temp()) {
            return false;
        }

        if (local_a->next && local_b->next)
            return is_within(local_a->next, local_b->next, true);

        return true;
    } else {
        // This is actually very reachable, but it's not implemented yet
        std::unreachable();
    }
}

void Analyser::print_lifetime_error(const aast::Expression *left,
                                    const aast::Expression *right,
                                    Lifetime *a,
                                    Lifetime *b,
                                    bool rec) const {
    if (a->is_local() && !b->is_local())
        return;

    if (!a->is_local() && b->is_local()) {
        bucket->note(current_function->statement_positions[((LocalLifetime *) b)->death-1],
                     "'{}' dies after this,...",
                     right->print());

        bucket->note(current_function->statement_positions.back(),
                     "...but '{}' outlives the function",
                     left->print());
        return;
    }

    if (a->is_local()) {
        auto *local_a = (LocalLifetime *) a;
        auto *local_b = (LocalLifetime *) b;

        if (rec && local_b->is_temp()) {
            bucket->note(current_function->statement_positions[local_a->death-1],
                     "'{}' dies after this,...",
                     left->print());

            bucket->note(current_function->statement_positions[local_b->death-1],
                         "...but '{}' takes a pointer of a temporary value which dies immediately",
                         right->print());
            return;
        } else if (local_a->death <= local_b->last_death) {
            // todo: properly order deaths
            local_a->last_death = std::min(local_a->last_death, local_b->last_death);
        } else if (!local_b->is_temp()) {
            bucket->note(current_function->statement_positions[local_b->death-1],
                     "'{}' dies after this,...",
                     right->print());

            bucket->note(current_function->statement_positions[local_a->death-1],
                         "...but '{}' lives until here",
                         left->print());
            return;
        }

        if (local_a->next && local_b->next)
            return print_lifetime_error(left, right->get_inner(), local_a->next, local_b->next, true);
    } else {
        // This is actually very reachable, but it's not implemented yet
        std::unreachable();
    }
}
} // lifetime

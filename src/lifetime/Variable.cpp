// tarik (c) Nikolas Wipper 2024

#include "Variable.h"

#include <print>

namespace lifetime
{
Lifetime::Lifetime(Lifetime *next)
    : next(next) {}

LocalLifetime::LocalLifetime(Lifetime *next, std::size_t at)
    : Lifetime(next),
      birth(at),
      death(at),
      last_death(0) {}

LocalLifetime::LocalLifetime(Lifetime *next, std::size_t birth, std::size_t deaths)
    : Lifetime(next),
      birth(birth),
      death(deaths),
      last_death(deaths) {}


LocalLifetime *LocalLifetime::static_(Lifetime *next) {
    return new LocalLifetime(next, 0, std::numeric_limits<std::size_t>::max());
}

LocalLifetime *LocalLifetime::temporary(Lifetime *next, std::size_t at) {
    auto *t = new LocalLifetime(next, at, at);
    t->temp = true;
    return t;
}

bool LocalLifetime::is_temp() const {
    return temp;
}

void LocalLifetime::set_last_death(std::size_t d) {
    last_death = d;
    if (next && next->is_local()) {
        auto *local = (LocalLifetime *) next;
        local->set_last_death(d);
    }
}

void LocalLifetime::set_last_death_safe(std::size_t d) {
    last_death = std::max(last_death, d);
    if (next && next->is_local()) {
        auto *local = (LocalLifetime *) next;
        local->set_last_death_safe(d);
    }
}

bool LocalLifetime::operator==(const LocalLifetime &o) const {
    return birth == o.birth && death == o.death && last_death == o.last_death;
}

VariableState::VariableState(LocalLifetime *type, std::size_t at)
    : lifetime(type),
      values({}) {}

void VariableState::used(std::size_t at, int depth) {
    if (values.empty()) {
        values.push_back(new LocalLifetime(nullptr, at));
    }
    values.back()->death = std::max(values.back()->death, at);

    lifetime->death = std::max(lifetime->death, at);

    Lifetime *lifetime_target = lifetime->next;
    Lifetime *value_target = values.back()->next;
    for (int i = 0; i < depth; i++) {
        if (lifetime_target->is_local()) {
            auto *lifetime_local = (LocalLifetime *) lifetime_target;
            auto *value_local = (LocalLifetime *) value_target;

            lifetime_local->death = std::max(lifetime_local->death, at);
            value_local->death = std::max(value_local->death, at);

            lifetime_target = lifetime_target->next;
            value_target = value_target->next;
        } else {
            break;
        }
    }
}

void VariableState::assigned(std::size_t at) {
    // If values last_death was set previously, because it was moved, don't overwrite that
    if (!values.empty() && values.back()->last_death == 0)
        values.back()->set_last_death(at - 1);

    auto *lt = new LocalLifetime(nullptr, at);
    Lifetime *target = lt;
    Lifetime *copy = lifetime->next;

    while (copy) {
        if (copy->is_local()) {
            target->next = new LocalLifetime(nullptr, at);
        } else {
            target->next = copy;
            break;
        }
        target = target->next;
        copy = copy->next;
    }

    values.push_back(lt);

    lifetime->death = at;
}

void VariableState::kill(std::size_t at) {
    // Killing ends the variables lifetime, and thus the values
    lifetime->set_last_death_safe(at);
    if (!values.empty())
        values.back()->set_last_death_safe(at);
}

void VariableState::move(std::size_t at) {
    // Moving only ends the values lifetime
    values.back()->set_last_death(at - 1);
}

LocalLifetime *VariableState::current(std::size_t at) {
    for (auto *lifetime : values) {
        if (lifetime->birth <= at && lifetime->last_death >= at) {
            return lifetime;
        }
    }
    return new LocalLifetime(nullptr, 0);
}

LocalLifetime *VariableState::current_continuous(std::size_t at) {
    LocalLifetime *res = nullptr;

    for (auto *lifetime : values) {
        if (lifetime->birth <= at && lifetime->death >= at) {
            res = lifetime;
        } else if (res && res->death == lifetime->birth) {
            res->death = lifetime->death;
            res->last_death = lifetime->last_death;
        }
    }
    if (res)
        return res;
    return new LocalLifetime(nullptr, 0);
}

CompoundState::CompoundState(LocalLifetime *type, std::size_t at, const std::vector<VariableState *> &states)
    : VariableState(type, at),
      children(states) {}

void CompoundState::used(std::size_t at, int depth) {
    VariableState::used(at, depth);

    for (auto *child : children)
        child->used(at, depth);
}

void CompoundState::assigned(std::size_t at) {
    VariableState::assigned(at);

    for (auto *child : children)
        child->assigned(at);
}

void CompoundState::kill(std::size_t at) {
    VariableState::kill(at);

    for (auto *child : children)
        child->kill(at);
}

void CompoundState::move(std::size_t at) {
    VariableState::move(at);

    for (auto *child : children)
        child->move(at);
}

LocalLifetime *CompoundState::current_continuous(std::size_t at) {
    std::vector<LocalLifetime *> children_lifetimes;

    children_lifetimes.reserve(children.size());
    for (auto *child : children)
        children_lifetimes.push_back(child->current_continuous(at));

    LocalLifetime *res = LocalLifetime::static_(nullptr);

    for (auto *lifetime : children_lifetimes) {
        res->birth = std::max(res->birth, lifetime->birth);
        res->death = std::min(res->death, lifetime->death);
        res->last_death = std::min(res->last_death, lifetime->last_death);
    }

    return res;
}
}

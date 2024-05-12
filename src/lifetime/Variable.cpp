// tarik (c) Nikolas Wipper 2024

#include "Variable.h"

namespace lifetime
{
VariableState::VariableState(std::size_t at)
    : lifetime(at),
      values({}) {}

void VariableState::used(std::size_t at) {
    if (values.empty()) {
        values.emplace_back(at);
    }
    values.back().death = std::max(values.back().death, at);

    lifetime.death = std::max(lifetime.death, at);
}

void VariableState::assigned(std::size_t at) {
    // If values last_death was set previously, because it was moved, don't overwrite that
    if (!values.empty() && values.back().last_death == 0)
        values.back().last_death = at;
    values.emplace_back(at);

    lifetime.death = at;
}

void VariableState::kill(std::size_t at) {
    // Killing ends the variables lifetime, and thus the values
    lifetime.last_death = std::max(lifetime.last_death, at);
    if (!values.empty())
        values.back().last_death = std::max(values.back().last_death, at);
}

void VariableState::move(std::size_t at) {
    // Moving only ends the values lifetime
    values.back().last_death = at;
}

Lifetime VariableState::current(std::size_t at) {
    for (auto &lifetime : values) {
        if (lifetime.birth <= at) {
            if (lifetime.last_death >= at)
                return lifetime;
        }
    }
    return Lifetime(0);
}

Lifetime VariableState::current_continuous(std::size_t at) {
    Lifetime res = Lifetime(0);
    bool found_current = false;

    for (auto &lifetime : values) {
        if (lifetime.birth <= at && lifetime.death >= at) {
            res = lifetime;
            found_current = true;
        } else if (found_current && res.death == lifetime.birth) {
            res.death = lifetime.death;
            res.last_death = lifetime.last_death;
        }
    }
    return res;
}

CompoundState::CompoundState(std::size_t at, const std::vector<VariableState *> &states)
    : VariableState(at),
      children(states) {}

void CompoundState::used(std::size_t at) {
    VariableState::used(at);

    for (auto *child : children)
        child->used(at);
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

Lifetime CompoundState::current_continuous(std::size_t at) {
    std::vector<Lifetime> children_lifetimes;

    children_lifetimes.reserve(children.size());
    for (auto *child : children)
        children_lifetimes.push_back(child->current_continuous(at));

    Lifetime res = Lifetime::static_();

    for (auto &lifetime : children_lifetimes) {
        res.birth = std::max(res.birth, lifetime.birth);
        res.death = std::min(res.death, lifetime.death);
        res.last_death = std::min(res.last_death, lifetime.last_death);
    }

    return res;
}
}

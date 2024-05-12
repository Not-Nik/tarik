// tarik (c) Nikolas Wipper 2024

#ifndef TARIK_SRC_LIFETIME_VARIABLE_H
#define TARIK_SRC_LIFETIME_VARIABLE_H

#include "Analyser.h"

namespace lifetime
{
struct VariableState {
    Lifetime lifetime;
    std::vector<Lifetime> values;

    explicit VariableState(std::size_t at);

    virtual void used(std::size_t at);
    virtual void assigned(std::size_t at);

    virtual void kill(std::size_t at);
    virtual void move(std::size_t at);

    virtual Lifetime current(std::size_t at);
    virtual Lifetime current_continuous(std::size_t at);
};

class CompoundState : public VariableState {
    std::vector<VariableState *> children;

public:
    CompoundState(std::size_t at, const std::vector<VariableState *> &states);

    void used(std::size_t at) override;
    void assigned(std::size_t at) override;

    void kill(std::size_t at) override;
    void move(std::size_t at) override;

    Lifetime current_continuous(std::size_t at) override;
};
}

#endif //TARIK_SRC_LIFETIME_VARIABLE_H

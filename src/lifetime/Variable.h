// tarik (c) Nikolas Wipper 2024

#ifndef TARIK_SRC_LIFETIME_VARIABLE_H
#define TARIK_SRC_LIFETIME_VARIABLE_H

#include <cstddef>
#include <vector>

namespace lifetime
{
struct Lifetime {
    Lifetime *next = nullptr;

    explicit Lifetime(Lifetime *next);

    virtual bool is_temp() const { return false; }
    virtual bool is_local() const { return false; }
};

struct LocalLifetime : Lifetime {
    std::size_t birth, death, last_death;
    bool temp = false;

    LocalLifetime(Lifetime *next, std::size_t at);
    static LocalLifetime *static_(Lifetime *next);
    static LocalLifetime *temporary(Lifetime *next, std::size_t at);

    bool is_temp() const override;
    bool is_local() const override { return true; }

    void set_last_death(std::size_t);
    void set_last_death_safe(std::size_t);

    bool operator==(const LocalLifetime &) const;

private:
    LocalLifetime(Lifetime *next, std::size_t birth, std::size_t deaths);
};

struct VariableState {
    LocalLifetime *lifetime;
    std::vector<LocalLifetime *> values;

    VariableState(LocalLifetime *type, std::size_t at);

    virtual void used(std::size_t at, int depth);
    virtual void assigned(std::size_t at);

    virtual void kill(std::size_t at);
    virtual void move(std::size_t at);

    virtual LocalLifetime *current(std::size_t at);
    virtual LocalLifetime *current_continuous(std::size_t at);
};

class CompoundState : public VariableState {
    std::vector<VariableState *> children;

public:
    CompoundState(LocalLifetime *type, std::size_t at, const std::vector<VariableState *> &states);

    void used(std::size_t at, int depth) override;
    void assigned(std::size_t at) override;

    void kill(std::size_t at) override;
    void move(std::size_t at) override;

    LocalLifetime *current_continuous(std::size_t at) override;
};
}

#endif //TARIK_SRC_LIFETIME_VARIABLE_H

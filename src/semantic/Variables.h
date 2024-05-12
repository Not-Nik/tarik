// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_SEMANTIC_VARIABLES_H
#define TARIK_SRC_SEMANTIC_VARIABLES_H

#include <stack>

#include "lexical/Token.h"
#include "semantic/ast/Statements.h"

class VariableState {
protected:
    bool is_undefined = true, was_defined = false, was_moved = false;
    LexerRange defined_pos, read_pos, moved_pos;

    VariableState(bool undefined, bool defined, bool moved, LexerRange defined_pos, LexerRange read_pos);

public:
    VariableState() = default;

    virtual void make_definitely_defined(LexerRange pos);
    virtual void make_definitely_read(LexerRange pos);
    virtual void make_definitely_moved(LexerRange pos);

    virtual bool is_definitely_undefined() const;
    virtual bool is_definitely_defined() const;
    virtual bool is_definitely_moved() const;

    virtual bool is_maybe_undefined() const;
    virtual bool is_maybe_defined() const;
    virtual bool is_maybe_moved() const;

    virtual LexerRange get_defined_pos() const;
    virtual LexerRange get_moved_pos() const;

    virtual VariableState operator||(const VariableState &other) const;
};

struct SemanticVariable {
    aast::VariableStatement *var;

    SemanticVariable(aast::VariableStatement *var)
        : var(var) {}

    virtual VariableState *state() = 0;
    virtual void pop_state() = 0;
    virtual void push_state(VariableState state) = 0;
};

struct CompoundState : VariableState {
    std::vector<SemanticVariable *> children;

    CompoundState(const std::vector<SemanticVariable *> &states);

    void make_definitely_defined(LexerRange pos) override;
    void make_definitely_read(LexerRange pos) override;
    void make_definitely_moved(LexerRange pos) override;

    bool is_definitely_undefined() const override;
    bool is_definitely_defined() const override;
    bool is_definitely_moved() const override;

    bool is_maybe_undefined() const override;
    bool is_maybe_defined() const override;
    bool is_maybe_moved() const override;

    LexerRange get_defined_pos() const override;
    LexerRange get_moved_pos() const override;

    VariableState operator||(const VariableState &other) const override;
};

class PrimitiveVariable : public SemanticVariable {
    std::stack<VariableState> state_stack;

public:
    PrimitiveVariable(aast::VariableStatement *var)
        : SemanticVariable(var),
          state_stack({VariableState()}) {}

    VariableState *state() override;
    void pop_state() override;
    void push_state(VariableState state) override;
};

class CompoundVariable : public SemanticVariable {
    CompoundState compound_state;

public:
    CompoundVariable(aast::VariableStatement *var, const std::vector<SemanticVariable *> &states)
        : SemanticVariable(var),
          compound_state(states) {}

    VariableState *state() override;
    void pop_state() override {};
    void push_state(VariableState _state) override {};
};

#endif //TARIK_SRC_SEMANTIC_VARIABLES_H

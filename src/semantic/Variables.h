// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef VARIABLES_H
#define VARIABLES_H
#include <stack>

#include "lexical/Token.h"
#include "syntactic/expressions/Statements.h"

class VariableState {
protected:
    bool is_undefined = true, was_defined = false, was_read = false;
    LexerRange defined_pos, read_pos;

    VariableState(bool undefined, bool defined, bool read, LexerRange defined_pos, LexerRange read_pos);

public:
    VariableState() = default;

    virtual void make_definitely_defined(LexerRange pos);
    virtual void make_definitely_read(LexerRange pos);

    virtual bool is_definitely_undefined() const;
    virtual bool is_definitely_defined() const;

    virtual bool is_maybe_undefined() const;
    virtual bool is_maybe_defined() const;

    virtual LexerRange get_defined_pos() const;
    virtual LexerRange get_read_pos() const;

    virtual VariableState operator||(const VariableState &other) const;
};

struct SemanticVariable {
    ast::VariableStatement *var;

    SemanticVariable(ast::VariableStatement *var)
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

    bool is_definitely_undefined() const override;
    bool is_definitely_defined() const override;

    bool is_maybe_undefined() const override;
    bool is_maybe_defined() const override;

    LexerRange get_defined_pos() const override;
    LexerRange get_read_pos() const override;

    VariableState operator||(const VariableState &other) const override;
};

class PrimitiveVariable : public SemanticVariable {
    std::stack<VariableState> state_stack;

public:
    PrimitiveVariable(ast::VariableStatement *var)
        : SemanticVariable(var),
          state_stack({VariableState()}) {}

    VariableState *state() override;
    void pop_state() override;
    void push_state(VariableState state) override;
};

class CompoundVariable : public SemanticVariable {
    CompoundState compound_state;

public:
    CompoundVariable(ast::VariableStatement *var, const std::vector<SemanticVariable *> &states)
        : SemanticVariable(var),
          compound_state(states) {}

    VariableState *state() override;
    void pop_state() override {};
    void push_state(VariableState _state) override {};
};

#endif //VARIABLES_H

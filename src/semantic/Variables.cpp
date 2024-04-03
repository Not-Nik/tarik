// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Variables.h"

VariableState::VariableState(bool undefined, bool defined, bool read, LexerRange defined_pos, LexerRange read_pos)
    : is_undefined(undefined),
      was_defined(defined),
      was_read(read),
      defined_pos(defined_pos),
      read_pos(read_pos) {}

void VariableState::make_definitely_defined(LexerRange pos) {
    is_undefined = false;
    was_defined = true;
    was_read = false;

    defined_pos = pos;
    read_pos = LexerRange();
}

void VariableState::make_definitely_read(LexerRange pos) {
    is_undefined = false;
    was_defined = false;
    was_read = true;

    defined_pos = LexerRange();
    read_pos = pos;
}

bool VariableState::is_definitely_undefined() const {
    return is_undefined && !was_defined && !was_read;
}

bool VariableState::is_definitely_defined() const {
    return !is_undefined && was_defined && !was_read;
}

bool VariableState::is_maybe_undefined() const {
    return is_undefined;
}

bool VariableState::is_maybe_defined() const {
    return was_defined;
}

LexerRange VariableState::get_defined_pos() const {
    return defined_pos;
}

LexerRange VariableState::get_read_pos() const {
    return read_pos;
}

VariableState VariableState::operator||(const VariableState &other) const {
    return VariableState(is_undefined || other.is_undefined,
                         was_defined || other.was_defined,
                         was_read || other.was_read,
                         (defined_pos > other.defined_pos ? defined_pos : other.defined_pos),
                         (read_pos > other.read_pos ? read_pos : other.read_pos));
}

CompoundState::CompoundState(const std::vector<SemanticVariable *> &states)
    : children(states) {}


void CompoundState::make_definitely_defined(LexerRange pos) {
    defined_pos = pos;
    for (auto child : children) {
        child->state()->make_definitely_defined(pos);
    }
}

void CompoundState::make_definitely_read(LexerRange pos) {
    read_pos = pos;
    for (auto child : children) {
        child->state()->make_definitely_read(pos);
    }
}

bool CompoundState::is_definitely_undefined() const {
    // empty structs are always defined
    if (children.empty()) return false;

    bool state = true;
    for (auto child : children) {
        state = state && child->state()->is_definitely_undefined();
    }
    return state;
}

bool CompoundState::is_definitely_defined() const {
    // empty structs are always defined, but can also be read, e.g., when they are assigned to another variable, i.e.
    // my_empty_struct mes = my_empty_struct();
    // my_empty_struct mes2 = mes; # <<-- mes is read here and now definitely not defined, i.e.
    //                             # is_definitely_undefined() = false
    //                             # is_definitely_defined() = false
    //                             # is_maybe_undefined() = false
    //                             # is_maybe_defined() = false
    // similarly empty structs can be read when they are returned
    bool state = true;
    for (auto child : children) {
        state = state && child->state()->is_definitely_defined();
    }
    return state;
}

bool CompoundState::is_maybe_undefined() const {
    bool state = false;
    for (auto child : children) {
        state = state || child->state()->is_maybe_undefined();
    }
    return state;
}

bool CompoundState::is_maybe_defined() const {
    bool state = false;
    for (auto child : children) {
        state = state || child->state()->is_maybe_defined();
    }
    return state;
}

LexerRange CompoundState::get_defined_pos() const {
    return defined_pos;
}

LexerRange CompoundState::get_read_pos() const {
    return read_pos;
}

VariableState CompoundState::operator||(const VariableState &other) const {
    return *this;
}

VariableState *PrimitiveVariable::state() {
    return &state_stack.top();
}

void PrimitiveVariable::pop_state() {
    state_stack.pop();
}

void PrimitiveVariable::push_state(VariableState state) {
    state_stack.push(state);
}

VariableState *CompoundVariable::state() {
    return &compound_state;
}

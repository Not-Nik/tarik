// tarik (c) Nikolas Wipper 2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Variables.h"

VariableState::VariableState(bool undefined,
                             bool defined,
                             bool moved,
                             LexerRange defined_pos)
    : is_undefined(undefined),
      was_defined(defined),
      was_moved(moved),
      defined_pos(defined_pos) {}

void VariableState::make_definitely_defined(LexerRange pos) {
    is_undefined = false;
    was_defined = true;
    was_moved = false;

    defined_pos = pos;
    moved_pos = LexerRange();
}

void VariableState::make_definitely_read() {
    is_undefined = false;
    was_defined = false;
    was_moved = false;

    defined_pos = LexerRange();
    moved_pos = LexerRange();
}

void VariableState::make_definitely_moved(LexerRange pos) {
    is_undefined = false;
    was_defined = false;
    was_moved = true;

    defined_pos = LexerRange();
    moved_pos = pos;
}

bool VariableState::is_definitely_undefined() const {
    return is_undefined && !was_defined && !was_moved;
}

bool VariableState::is_definitely_defined() const {
    return !is_undefined && was_defined && !was_moved;
}

bool VariableState::is_definitely_moved() const {
    return !is_undefined && !was_defined && was_moved;
}

bool VariableState::is_maybe_undefined() const {
    return is_undefined;
}

bool VariableState::is_maybe_defined() const {
    return was_defined;
}

bool VariableState::is_maybe_moved() const {
    return was_moved;
}

LexerRange VariableState::get_defined_pos() const {
    return defined_pos;
}

LexerRange VariableState::get_moved_pos() const {
    return moved_pos;
}

VariableState VariableState::operator||(const VariableState &other) const {
    return VariableState(is_undefined || other.is_undefined,
                         was_defined || other.was_defined,
                         was_moved || other.was_moved,
                         (defined_pos > other.defined_pos ? defined_pos : other.defined_pos));
}

CompoundState::CompoundState(const std::vector<SemanticVariable *> &states)
    : children(states) {}


void CompoundState::make_definitely_defined(LexerRange pos) {
    VariableState::make_definitely_defined(pos);
    for (auto *child : children) {
        child->state()->make_definitely_defined(pos);
    }
}

void CompoundState::make_definitely_read() {
    VariableState::make_definitely_read();
    for (auto *child : children) {
        child->state()->make_definitely_read();
    }
}

void CompoundState::make_definitely_moved(LexerRange pos) {
    VariableState::make_definitely_moved(pos);
    for (auto *child : children) {
        child->state()->make_definitely_moved(pos);
    }
}

bool CompoundState::is_definitely_undefined() const {
    bool state = false;
    for (auto *child : children) {
        state = state || child->state()->is_definitely_undefined();
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
    for (auto *child : children) {
        state = state && child->state()->is_definitely_defined();
    }
    return state;
}

bool CompoundState::is_definitely_moved() const {
    bool state = VariableState::is_definitely_moved();
    // if any of the structs children were moved, the struct is also
    // moved or, more specifically, is not available for moving anymore
    for (auto *child : children) {
        state = state || child->state()->is_definitely_moved();
    }
    return state;
}

bool CompoundState::is_maybe_undefined() const {
    bool state = false;
    for (auto *child : children) {
        state = state || child->state()->is_maybe_undefined();
    }
    return state;
}

bool CompoundState::is_maybe_defined() const {
    bool state = false;
    for (auto *child : children) {
        state = state || child->state()->is_maybe_defined();
    }
    return state;
}

bool CompoundState::is_maybe_moved() const {
    bool state = was_moved;
    for (auto *child : children) {
        state = state || child->state()->is_maybe_moved();
    }
    return state;
}

LexerRange CompoundState::get_defined_pos() const {
    return defined_pos;
}

LexerRange CompoundState::get_moved_pos() const {
    LexerRange pos = moved_pos;
    for (auto *child : children) {
        if (!child->state()->is_maybe_moved()) continue;
        LexerRange child_pos = child->state()->get_moved_pos();
        if (child->state()->get_moved_pos() > pos)
            pos = child_pos;
    }
    return pos;
}

VariableState CompoundState::operator||(const VariableState &) const {
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

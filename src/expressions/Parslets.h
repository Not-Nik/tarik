// Seno (c) Nikolas Wipper 2020

#ifndef SENO_PARSLETS_H
#define SENO_PARSLETS_H

#include "Expression.h"
#include "../Token.h"

class Parser;

class PrefixParselet {
public:
    virtual Expression parse(const Parser * parser, const Token & token) { return {}; }
};

class InfixParselet {
public:
    virtual Expression parse(const Parser * parser, const Expression & left, const Token & token) { return {}; }

    virtual ExprType get_type() { return static_cast<ExprType>(0); }
};

#endif //SENO_PARSLETS_H

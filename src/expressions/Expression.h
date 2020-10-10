// Seno (c) Nikolas Wipper 2020

#ifndef SENO_EXPRESSION_H
#define SENO_EXPRESSION_H

enum ExprType {
    CALL_EXPR,
    DASH_EXPR, // add subtract
    DOT_EXPR, // multiply divide
    PREFIX_EXPR,
    ASSIGN_EXPR,
};

class Expression {
public:
    ExprType type;
};


#endif //SENO_EXPRESSION_H

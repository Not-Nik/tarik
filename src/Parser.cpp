// tarik (c) Nikolas Wipper 2020

#include "Parser.h"

#include "expressions/Parslets.h"

#define STDERR_WARN

#include <comperr.h>

#include <utility>

void Parser::iassert(bool cond, std::string what, ...) {
    va_list args;
    va_start(args, what);
    vcomperr(cond, what.c_str(), false, filename.c_str(), lexer.where().l, lexer.where().p, args);
    va_end(args);
}

ExprType Parser::get_precedence() {
    if (lexer.peek().id == SEMICOLON) {
        return (ExprType) -1;
    }
    if (infix_parslets.count(lexer.peek().id) > 0)
        return infix_parslets[lexer.peek().id]->get_type();
    return static_cast<ExprType>(0);
}

std::vector<Statement *> Parser::block() {
    expect(CURLY_OPEN);
    std::vector < Statement * > res;
    while (lexer.peek().id != CURLY_CLOSE) {
        Statement * statement = parse_statement();
        if (res.back()->statement_type != IF_STMT) {
            iassert(statement->statement_type != ELSE_STMT, "Else without matching if");
        } else if (statement->statement_type == ELSE_STMT) {
            ((ElseStatement *) statement)->inverse = (IfStatement *) res.back();
        }
        res.push_back(parse_statement());
        expect(SEMICOLON);
    }
    expect(CURLY_CLOSE);
    return res;
}

Type Parser::type() {
    iassert(type_names.count(lexer.peek().raw) > 0, "Expected type name");
    Type tmp = type_names[lexer.consume().raw];
    while (lexer.peek().id == ASTERISK) {
        tmp.pointer_level++;
        lexer.consume();
    }
    return tmp;
}

Parser::Parser(std::string code, std::string fn) :
        lexer(std::move(code)),
        filename(std::move(fn)) {
    // Trivial expressions
    prefix_parslets.emplace(NAME, new NameParselet());
    prefix_parslets.emplace(INTEGER, new IntParselet());
    prefix_parslets.emplace(REAL, new RealParselet());

    // Prefix expressions
    prefix_parslets.emplace(PLUS, new PosParselet());
    prefix_parslets.emplace(MINUS, new NegParselet());
    prefix_parslets.emplace(PAREN_OPEN, new GroupParselet());

    // Binary expressions
    infix_parslets.emplace(PLUS, new AddParselet());
    infix_parslets.emplace(MINUS, new SubParselet());
    infix_parslets.emplace(ASTERISK, new MulParselet());
    infix_parslets.emplace(SLASH, new DivParselet());

    type_names.emplace("i8", Type(TypeUnion {.size = INT8}, true, false));
    type_names.emplace("u8", Type(TypeUnion {.size = UINT8}, true, false));
    type_names.emplace("i16", Type(TypeUnion {.size = INT16}, true, false));
    type_names.emplace("u16", Type(TypeUnion {.size = UINT16}, true, false));
    type_names.emplace("i32", Type(TypeUnion {.size = INT32}, true, false));
    type_names.emplace("u32", Type(TypeUnion {.size = UINT32}, true, false));
    type_names.emplace("i64", Type(TypeUnion {.size = INT64}, true, false));
    type_names.emplace("u64", Type(TypeUnion {.size = UINT64}, true, false));
    type_names.emplace("f32", Type(TypeUnion {.size = FLOAT32}, true, false));
    type_names.emplace("f64", Type(TypeUnion {.size = FLOAT64}, true, false));
}

Parser::~Parser() {
    endfile();
}

Token Parser::expect(TokenType raw) {
    iassert(lexer.peek().id == raw, "Unexpected '%s'.", lexer.peek().raw.c_str());
    return lexer.consume();
}

void Parser::expect(const std::string & raw) {
    iassert(lexer.peek().raw == raw, "Expected '%s' found '%s'.", raw.c_str(), lexer.peek().raw.c_str());
    lexer.consume();
}

Expression * Parser::parse_expression(int precedence) {
    Token token = lexer.consume();
    iassert(prefix_parslets.count(token.id) > 0, "Unexpected token '%s'", token.raw.c_str());
    PrefixParselet * prefix = prefix_parslets[token.id];

    Expression * left = prefix->parse(this, token);

    while (precedence < get_precedence()) {
        token = lexer.consume();

        InfixParselet * infix = infix_parslets[token.id];
        left = infix->parse(this, left, token);
    }

    return left;
}

Statement * Parser::parse_statement() {
    TokenType token = lexer.peek().id;
    if (token == FUNC) {
        lexer.consume();
        std::string name = expect(NAME).raw;
        std::map <std::string, Type> args;
        expect(PAREN_OPEN);
        while (lexer.peek().id != PAREN_CLOSE) {
            Type t = type();
            args.emplace(expect(NAME).raw, t);
            if (lexer.peek().id != PAREN_CLOSE)
                expect(COMMA);
        }
        expect(PAREN_CLOSE);
        return new FuncStatement(name, type(), args, block());
    } else if (token == RETURN) {
        lexer.consume();
        return new ReturnStatement(parse_expression());
    } else if (token == IF) {
        lexer.consume();
        return new IfStatement(parse_expression(), block());
    } else if (token == ELSE) {
        lexer.consume();
        return new ElseStatement(nullptr, block());
    } else if (token == WHILE) {
        lexer.consume();
        return new WhileStatement(parse_expression(), block());
    } else if (token == FOR) {

    }
    return parse_expression();
}

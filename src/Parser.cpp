// tarik (c) Nikolas Wipper 2020

#include "Parser.h"

#include "expressions/Parslets.h"

#include <comperr.h>

#include <utility>

Precedence Parser::get_precedence() {
    if (lexer.peek().id == SEMICOLON) {
        return static_cast<Precedence>(-1);
    }
    if (infix_parslets.count(lexer.peek().id) > 0)
        return infix_parslets[lexer.peek().id]->get_type();
    return static_cast<Precedence>(0);
}

std::vector<Statement *> Parser::block() {
    if (!check_expect(CURLY_OPEN))
        return {};
    std::vector<Statement *> res;
    variable_pop_stack.push_back(0);
    while (!lexer.peek().raw.empty() && lexer.peek().id != CURLY_CLOSE) {
        Statement *statement = parse_statement(false);
        if (!res.empty() && res.back()->statement_type != IF_STMT) {
            iassert(statement->statement_type != ELSE_STMT, "Else without matching if");
        } else if (statement->statement_type == ELSE_STMT) {
        }
        res.push_back(statement);
    }
    for (int i = 0; i < variable_pop_stack.back(); i++)
        variables.pop_back();
    variable_pop_stack.pop_back();
    expect(CURLY_CLOSE);
    return res;
}

Statement *Parser::scope() {
    return new ScopeStatement(SCOPE_STMT, lexer.where(), block());
}

Type Parser::type() {
    iassert(lexer.peek().id == TYPE or lexer.peek().id == USER_TYPE, "Expected type name");
    Type t;

    if (lexer.peek().id == TYPE) {
        std::string type_name;
        for (auto c: lexer.peek().raw)
            type_name.push_back((char) toupper(c));
        TypeSize size = to_typesize(type_name);
        iassert(size != (TypeSize) -1, "Internal: Couldn't find enum member for built-in type");
        t.type.size = size;
    } else {
        StructStatement *structure;
        for (StructStatement *st: structures) {
            if (st->name == lexer.peek().raw) {
                structure = st;
                break;
            }
        }
        iassert(structure, "Undefined structure %s", lexer.peek().raw.c_str());
        t.type.user_type = structure;
        t.is_primitive = false;
    }
    lexer.consume();
    while (!lexer.peek().raw.empty() && lexer.peek().id == ASTERISK) {
        t.pointer_level++;
        lexer.consume();
    }
    return t;
}

Parser::Parser(std::istream *code, std::string fn)
    : lexer(code), filename(std::move(fn)) {
    // Trivial expressions
    prefix_parslets.emplace(NAME, new NameParselet());
    prefix_parslets.emplace(INTEGER, new IntParselet());
    prefix_parslets.emplace(REAL, new RealParselet());

    // Prefix expressions
    prefix_parslets.emplace(PLUS, new PosParselet());
    prefix_parslets.emplace(MINUS, new NegParselet());
    prefix_parslets.emplace(ASTERISK, new DerefParselet());
    prefix_parslets.emplace(NOT, new NotParselet());
    prefix_parslets.emplace(PAREN_OPEN, new GroupParselet());

    // Binary expressions
    infix_parslets.emplace(PLUS, new AddParselet());
    infix_parslets.emplace(MINUS, new SubParselet());
    infix_parslets.emplace(ASTERISK, new MulParselet());
    infix_parslets.emplace(SLASH, new DivParselet());
    infix_parslets.emplace(DOUBLE_EQUAL, new EqParselet());
    infix_parslets.emplace(NOT_EQUAL, new NeqParselet());
    infix_parslets.emplace(SMALLER, new SmParselet());
    infix_parslets.emplace(GREATER, new GrParselet());
    infix_parslets.emplace(SMALLER_EQUAL, new SeParselet());
    infix_parslets.emplace(GREATER_EQUAL, new GeParselet());

    // Call
    infix_parslets.emplace(PAREN_OPEN, new CallParselet());

    // Assign expressions
    infix_parslets.emplace(EQUAL, new AssignParselet());

    variable_pop_stack.push_back(0);
}

Parser::~Parser() {
    for (auto pre: prefix_parslets)
        delete pre.second;
    for (auto in: infix_parslets)
        delete in.second;
    endfile();
}

bool Parser::iassert(bool cond, std::string what, ...) {
    va_list args;
    va_start(args, what);
    vcomperr(cond, what.c_str(), false, filename.c_str(), lexer.where().l, lexer.where().p, args);
    va_end(args);
    return cond;
}

bool Parser::iassert(bool cond, LexerPos pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    vcomperr(cond, what.c_str(), false, filename.c_str(), pos.l, pos.p, args);
    va_end(args);
    return cond;
}

Token Parser::expect(TokenType raw) {
    std::string s = to_string(raw);
    iassert(lexer.peek().id == raw, "Expected a %s found '%s' instead", s.c_str(), lexer.peek().raw.c_str());
    return lexer.consume();
}

bool Parser::is_peek(TokenType raw) {
    return lexer.peek().id == raw;
}

bool Parser::check_expect(TokenType raw) {
    std::string s = to_string(raw);
    bool r = iassert(lexer.peek().id == raw, "Expected a %s found '%s' instead", s.c_str(), lexer.peek().raw.c_str());
    lexer.consume();
    return r;
}

VariableStatement *Parser::require_var(const std::string &name) {
    for (auto *var: variables) {
        if (var->name == name)
            return var;
    }
    iassert(false, "Undefined variable %s", name.c_str());
    return nullptr;
}

VariableStatement *Parser::register_var(VariableStatement *var) {
    variable_pop_stack.back()++;
    variables.push_back(var);
    return variables.back();
}

FuncStatement *Parser::require_func(const std::string &name) {
    for (auto *func: functions) {
        if (func->name == name)
            return func;
    }
    iassert(false, "Undefined function %s", name.c_str());
    return nullptr;
}

FuncStatement *Parser::register_func(FuncStatement *func) {
    for (auto f: functions) {
        if (!iassert(f->name != func->name, func->origin, "Redefinition of '%s'; as '%s'", f->name.c_str(), func->signature().c_str())) {
            return nullptr;
        }
    }
    functions.push_back(func);
    return func;
}

Expression *Parser::parse_expression(int precedence) {
    Token token = lexer.consume();
    if (token.raw.empty()) return reinterpret_cast<Expression *>(-1);
    if (!iassert(prefix_parslets.count(token.id) > 0, "Unexpected token '%s'", token.raw.c_str()))
        return nullptr;
    PrefixParselet *prefix = prefix_parslets[token.id];

    Expression *left = prefix->parse(this, token);

    while (precedence < get_precedence()) {
        token = lexer.consume();

        InfixParselet *infix = infix_parslets[token.id];
        left = infix->parse(this, left, token);
    }

    iassert(left->expression_type != NAME_EXPR, "Internal: returned name expression. Please report this bug at the tarik repo.");

    return left;
}

Statement *Parser::parse_statement(bool top_level) {
    TokenType token = lexer.peek().id;
    if (token == END) return nullptr;

    if (token != FUNC && token != TYPE && token != USER_TYPE && token != STRUCT)
        iassert(!top_level, "Expected function, struct or variable definition");

    if (token == FUNC) {
        iassert(top_level, "Function definition is not allowed here");
        lexer.consume();
        std::string name = expect(NAME).raw;

        std::vector<VariableStatement *> args;
        if (!check_expect(PAREN_OPEN))
            while (!lexer.peek().raw.empty() && lexer.peek().id != PAREN_CLOSE) { lexer.consume(); }
        else
            while (!lexer.peek().raw.empty() && lexer.peek().id != PAREN_CLOSE) {
                args.push_back(register_var(new VariableStatement(lexer.where(), type(), expect(NAME).raw)));

                if (lexer.peek().id != PAREN_CLOSE)
                    expect(COMMA);
            }
        expect(PAREN_CLOSE);
        FuncStatement *fs = register_func(new FuncStatement(--lexer.where(), name, type(), args, {}));
        fs->block = block();
        return fs;
    } else if (token == RETURN) {
        lexer.consume();
        auto *s = new ReturnStatement(lexer.where(), parse_expression());
        expect(SEMICOLON);
        return s;
    } else if (token == IF) {
        lexer.consume();
        return new IfStatement(lexer.where(), parse_expression(), scope());
    } else if (token == ELSE) {
        lexer.consume();
        return new ElseStatement(lexer.where(), nullptr, scope());
    } else if (token == WHILE) {
        lexer.consume();
        return new WhileStatement(lexer.where(), parse_expression(), scope());
    } else if (token == CURLY_OPEN) {
        return new ScopeStatement(SCOPE_STMT, lexer.where(), block());
    } else if (token == TYPE or token == USER_TYPE) {
        Type t = type();

        iassert(is_peek(NAME), "Expected a NAME found '%s' instead", lexer.peek().raw.c_str());
        std::string name = lexer.peek().raw;

        if (lexer.peek(1).id != EQUAL) {
            lexer.consume();
            expect(SEMICOLON);
        }

        return register_var(new VariableStatement(lexer.where(), t, name));
    }
    Expression *e = parse_expression();
    if (e == reinterpret_cast<Expression *>(-1)) {
        return nullptr;
    } else if (e == nullptr) {
        return parse_statement(top_level);
    } else {
        expect(SEMICOLON);
        return e;
    }
}

int Parser::error_count() {
    return errorcount();
}

// tarik (c) Nikolas Wipper 2020

#include "Parser.h"

#include "util/Util.h"
#include "expressions/Parslets.h"

#include <comperr.h>

#include <fstream>
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
        Statement *statement = parse_statement();
        res.push_back(statement);
    }
    for (int i = 0; i < variable_pop_stack.back(); i++)
        variables.pop_back();
    variable_pop_stack.pop_back();
    expect(CURLY_CLOSE);
    return res;
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

void Parser::init_parslets() {
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

Parser::Parser(std::istream *code)
    : lexer(code) {
    init_parslets();
}

Parser::Parser(const std::filesystem::path &f)
    : lexer(f) {
    init_parslets();
}

Parser::~Parser() {
    for (auto pre: prefix_parslets)
        delete pre.second;
    for (auto in: infix_parslets)
        delete in.second;
}

bool Parser::iassert(bool cond, std::string what, ...) {
    va_list args;
    va_start(args, what);
    ::iassert(cond, where(), what, args);
    va_end(args);
    return cond;
}

void Parser::warn(std::string what, ...) {
    va_list args;
    va_start(args, what);
    ::warning_v(where(), what, args);
    va_end(args);
}

LexerPos Parser::where() {
    return lexer.where();
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

VariableStatement *Parser::register_var(VariableStatement *var) {
    variable_pop_stack.back()++;
    variables.push_back(var);
    return variables.back();
}

FuncStatement *Parser::register_func(FuncStatement *func) {
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
        left = infix->parse(this, left);
    }

    return left;
}

Statement *Parser::parse_statement() {
    Token token = lexer.peek();
    if (token.id == END) return nullptr;

    if (token.id == FUNC) {
        lexer.consume();
        std::string name = expect(NAME).raw;

        std::vector<VariableStatement *> args;
        if (!check_expect(PAREN_OPEN))
            while (!lexer.peek().raw.empty() && lexer.peek().id != PAREN_CLOSE) { lexer.consume(); }
        else
            while (!lexer.peek().raw.empty() && lexer.peek().id != PAREN_CLOSE) {
                args.push_back(register_var(new VariableStatement(where(), type(), expect(NAME).raw)));

                if (lexer.peek().id != PAREN_CLOSE)
                    expect(COMMA);
            }
        expect(PAREN_CLOSE);
        Type t;
        if (lexer.peek().id == CURLY_OPEN || lexer.peek().id == SEMICOLON) t = Type(VOID);
        else t = type();

        if (lexer.peek().id == SEMICOLON) {lexer.consume(); return new FuncDeclareStatement(--where(), name, t, args);}

        FuncStatement *fs = register_func(new FuncStatement(--where(), name, t, args, {}));
        fs->block = block();
        return fs;
    } else if (token.id == RETURN) {
        lexer.consume();
        auto *s = new ReturnStatement(where(), parse_expression());
        expect(SEMICOLON);
        return s;
    } else if (token.id == IF) {
        lexer.consume();
        auto is = new IfStatement(where(), parse_expression(), block());
        if (lexer.peek().id == ELSE) {
            auto es = parse_statement();
            iassert(es->statement_type == ELSE_STMT, "Internal: Next token is 'else', but parsed statement isn't. Report this as a bug");
            is->else_statement = (ElseStatement *) es;
        }
        return is;
    } else if (token.id == ELSE) {
        lexer.consume();
        return new ElseStatement(where(), block());
    } else if (token.id == WHILE) {
        lexer.consume();
        return new WhileStatement(where(), parse_expression(), block());
    } else if (token.id == BREAK) {
        lexer.consume();
        expect(SEMICOLON);
        return new BreakStatement(token.where);
    } else if (token.id == CONTINUE) {
        lexer.consume();
        expect(SEMICOLON);
        return new ContinueStatement(token.where);
    } else if (token.id == CURLY_OPEN) {
        return new ScopeStatement(SCOPE_STMT, where(), block());
    } else if (token.id == TYPE or token.id == USER_TYPE) {
        Type t = type();

        iassert(is_peek(NAME), "Expected an identifier found '%s' instead", lexer.peek().raw.c_str());
        std::string name = lexer.peek().raw;

        if (lexer.peek(1).id != EQUAL) {
            lexer.consume();
            expect(SEMICOLON);
        }

        return register_var(new VariableStatement(where(), t, name));
    }
    Expression *e = parse_expression();
    if (e == reinterpret_cast<Expression *>(-1)) {
        return nullptr;
    } else if (e == nullptr) {
        return parse_statement();
    } else {
        expect(SEMICOLON);
        return e;
    }
}

int Parser::error_count() {
    return errorcount();
}

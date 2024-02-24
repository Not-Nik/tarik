// tarik (c) Nikolas Wipper 2020-2023

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Parser.h"

#include "util/Util.h"
#include "expressions/Parslets.h"

#include <comperr.h>

#include <algorithm>

std::vector<std::filesystem::path> Parser::imported;

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
    while (lexer.peek().id != END && lexer.peek().id != CURLY_CLOSE) {
        Statement *statement = parse_statement();
        res.push_back(statement);
    }
    expect(CURLY_CLOSE);
    return res;
}

std::optional<Type> Parser::type() {
    int peek_distance = 0;

    auto peek = lexer.peek(peek_distance++);
    if (peek.id != TYPE && peek.id != NAME)
        return {};
    Type t;

    if (peek.id == TYPE) {
        std::string type_name;
        for (auto c : peek.raw)
            type_name.push_back((char) toupper(c));
        TypeSize size = to_typesize(type_name);
        iassert(size != (TypeSize) -1, "internal: couldn't find enum member for built-in type");
        t = Type(size);
    } else {
        std::vector path = {peek.raw};

        while (lexer.peek(peek_distance++).id == DOUBLE_COLON) {
            Token part = lexer.peek(peek_distance++);
            if (part.id == NAME)
                path.push_back(part.raw);
            else
                peek_distance--;
        }

        peek_distance--;

        t = Type(path);
    }
    while (lexer.peek(peek_distance++).id == ASTERISK) {
        t.pointer_level++;
    }

    peek_distance--;

    for (int _ = 0; _ < peek_distance; _++)
        lexer.consume();

    return t;
}

void Parser::init_parslets() {
    // Trivial expressions
    prefix_parslets.emplace(NAME, new NameParselet());
    prefix_parslets.emplace(INTEGER, new IntParselet());
    prefix_parslets.emplace(REAL, new RealParselet());
    prefix_parslets.emplace(STRING, new StringParselet());
    prefix_parslets.emplace(NULL_, new NullParselet());
    // This uses a tiny bit more memory, but saves us from a double free mess
    prefix_parslets.emplace(TRUE, new BoolParselet());
    prefix_parslets.emplace(FALSE, new BoolParselet());

    // Prefix expressions
    prefix_parslets.emplace(PLUS, new PosParselet());
    prefix_parslets.emplace(MINUS, new NegParselet());
    prefix_parslets.emplace(AMPERSAND, new RefParselet());
    prefix_parslets.emplace(ASTERISK, new DerefParselet());
    prefix_parslets.emplace(NOT, new NotParselet());
    prefix_parslets.emplace(DOUBLE_COLON, new GlobalParselet());
    prefix_parslets.emplace(PAREN_OPEN, new GroupParselet());

    // Binary expressions
    infix_parslets.emplace(DOUBLE_COLON, new PathParselet());
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
    infix_parslets.emplace(PERIOD, new MemberAccessParselet());

    // Call
    infix_parslets.emplace(PAREN_OPEN, new CallParselet());

    // Assign expressions
    infix_parslets.emplace(EQUAL, new AssignParselet());
}

Parser::Parser(std::istream *code, std::vector<std::filesystem::path> paths)
    : lexer(code),
      search_paths(std::move(paths)) {
    init_parslets();
}

Parser::Parser(const std::filesystem::path &f, std::vector<std::filesystem::path> paths)
    : lexer(f),
      search_paths(std::move(paths)) {
    imported.push_back(absolute(f));
    init_parslets();
}

Parser::~Parser() {
    for (auto pre : prefix_parslets)
        delete pre.second;
    for (auto in : infix_parslets)
        delete in.second;
}

bool Parser::iassert(bool cond, std::string what, ...) {
    va_list args;
    va_start(args, what);
    ::iassert(cond, where(), what, args);
    va_end(args);
    if (!cond)
        lexer.read_until({';', '}'});
    return cond;
}

LexerPos Parser::where() {
    return lexer.where();
}

Token Parser::expect(TokenType raw) {
    std::string s = to_string(raw);
    iassert(lexer.peek().id == raw, "expected a %s found '%s' instead", s.c_str(), lexer.peek().raw.c_str());
    return lexer.consume();
}

bool Parser::check_expect(TokenType raw) {
    std::string s = to_string(raw);
    bool r = iassert(lexer.peek().id == raw, "expected a %s found '%s' instead", s.c_str(), lexer.peek().raw.c_str());
    lexer.consume();
    return r;
}

bool Parser::is_peek(TokenType raw) {
    return lexer.peek().id == raw;
}

std::filesystem::path Parser::find_import() {
    Token next = expect(NAME);

    std::filesystem::path import_;

    while (next.id == NAME) {
        import_ /= next.raw;
        if (is_peek(PERIOD)) {
            lexer.consume();
            next = expect(NAME);
        } else
            break;
    }

    import_.replace_extension(".tk");

    if (exists(import_))
        return import_;

    for (const auto &path : search_paths) {
        if (exists(path / import_)) {
            return path / import_;
        }
    }
    return import_;
}

Expression *Parser::parse_expression(int precedence) {
    Token token = lexer.peek();
    if (token.id == END)
        return reinterpret_cast<Expression *>(-1);
    if (prefix_parslets.count(token.id) == 0)
        return nullptr;
    lexer.consume();
    PrefixParselet *prefix = prefix_parslets[token.id];

    Expression *left = prefix->parse(this, token);

    while (precedence < get_precedence()) {
        token = lexer.consume();

        InfixParselet *infix = infix_parslets[token.id];
        left = infix->parse(this, token, left);
    }

    return left;
}

Statement *Parser::parse_statement() {
    Token token = lexer.peek();
    if (token.id == END)
        return nullptr;

    if (token.id == FUNC) {
        lexer.consume();
        Token name = expect(NAME);

        std::vector<VariableStatement *> args;
        bool var_arg = false;

        if (!check_expect(PAREN_OPEN)) {
            while (lexer.peek().id != END && lexer.peek().id != PAREN_CLOSE)
                lexer.consume();
        } else {
            while (lexer.peek().id != END && lexer.peek().id != PAREN_CLOSE) {
                if (lexer.peek().id == TRIPLE_PERIOD) {
                    var_arg = true;
                    lexer.consume();
                    break;
                }
                std::optional<Type> ty = type();
                iassert(ty.has_value(), "exepcted type name");

                args.push_back(new VariableStatement(where(), ty.value_or(Type()), expect(NAME).raw));

                if (lexer.peek().id != PAREN_CLOSE)
                    expect(COMMA);
            }
        }
        expect(PAREN_CLOSE);
        Type t;
        if (lexer.peek().id == CURLY_OPEN)
            t = Type(VOID);
        else {
            std::optional<Type> ty = type();
            iassert(ty.has_value(), "exepcted type name");
            t = ty.value_or(Type());
        }

        std::vector<Statement *> body = block();

        return new FuncStatement(token.where, name, t, args, body, var_arg);
    } else if (token.id == RETURN) {
        lexer.consume();
        auto *s = new ReturnStatement(token.where, parse_expression());
        expect(SEMICOLON);
        return s;
    } else if (token.id == IF) {
        lexer.consume();
        auto is = new IfStatement(token.where, parse_expression(), block());
        if (lexer.peek().id == ELSE) {
            auto es = parse_statement();
            iassert(es->statement_type == ELSE_STMT,
                    "internal: next token is 'else', but parsed statement isn't. report this as a bug");
            is->else_statement = (ElseStatement *) es;
        }
        return is;
    } else if (token.id == ELSE) {
        lexer.consume();
        return new ElseStatement(token.where, block());
    } else if (token.id == WHILE) {
        lexer.consume();
        return new WhileStatement(token.where, parse_expression(), block());
    } else if (token.id == BREAK) {
        lexer.consume();
        expect(SEMICOLON);
        return new BreakStatement(token.where);
    } else if (token.id == CONTINUE) {
        lexer.consume();
        expect(SEMICOLON);
        return new ContinueStatement(token.where);
    } else if (token.id == CURLY_OPEN) {
        return new ScopeStatement(SCOPE_STMT, token.where, block());
    } else if (token.id == STRUCT) {
        lexer.consume();

        Token name = expect(NAME);
        expect(CURLY_OPEN);

        std::vector<VariableStatement *> members;

        while (lexer.peek().id != CURLY_CLOSE) {
            std::optional<Type> member_type = type();

            iassert(member_type.has_value(), "expected type name");

            std::string member_name = expect(NAME).raw;

            members.push_back(new VariableStatement(where(), member_type.value_or(Type()), member_name));
            expect(SEMICOLON);
        }
        lexer.consume();

        return new StructStatement(token.where, name, members);
    } else if (token.id == IMPORT) {
        lexer.consume();

        auto import_path = find_import();
        std::vector<Statement *> statements;
        if (exists(import_path)) {
            if (std::find(imported.begin(), imported.end(), absolute(import_path)) == imported.end()) {
                imported.push_back(absolute(import_path));
                Parser p(import_path, search_paths);
                do {
                    statements.push_back(p.parse_statement());
                } while (statements.back());
                statements.pop_back();
            }
        } else {
            iassert(false, "tried to import '%s', but file can't be found", import_path.string().c_str());
        }
        expect(SEMICOLON);

        ImportStatement *res;
        for (auto part = --import_path.end(); ; part--) {
            res = new ImportStatement(token.where, part->stem(), statements);
            statements = {res};

            if (part == import_path.begin())
                break;
        }
        return res;
    } else if (lexer.peek(1).id != EQUAL) {
        if (std::optional<Type> ty = type(); ty.has_value()) {
            Type t = ty.value();

            iassert(is_peek(NAME), "expected a name found '%s' instead", lexer.peek().raw.c_str());
            std::string name = lexer.peek().raw;

            LexerPos pos = lexer.where();

            if (lexer.peek(1).id != EQUAL) {
                lexer.consume();
                expect(SEMICOLON);
            }

            return new VariableStatement(pos, t, name);
        }
    }

    Expression *e = parse_expression();
    if (e == reinterpret_cast<Expression *>(-1)) {
        return nullptr;
    } else if (e != nullptr) {
        expect(SEMICOLON);
        return e;
    } else {
        iassert(false, "unexpected token '%s'", token.raw.c_str());
        return parse_statement();
    }
}

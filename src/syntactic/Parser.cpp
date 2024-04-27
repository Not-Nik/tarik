// tarik (c) Nikolas Wipper 2020-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Parser.h"

#include "ast/Parslets.h"

using namespace ast;

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
    if (peek.id != TYPE && peek.id != NAME && peek.id != DOUBLE_COLON)
        return {};
    Type t;

    if (peek.id == TYPE) {
        std::string type_name;
        for (auto c : peek.raw)
            type_name.push_back((char) toupper(c));
        TypeSize size = to_typesize(type_name);
        bucket->iassert(size != (TypeSize) -1, peek.origin, "internal: couldn't find enum member for built-in type");
        t = Type(size);
    } else {
        std::vector path = {peek.raw};
        if (peek.id == DOUBLE_COLON) {
            path = {""};
            peek_distance--;
        }

        while (lexer.peek(peek_distance++).id == DOUBLE_COLON) {
            Token part = lexer.peek(peek_distance++);
            if (part.id == NAME)
                path.push_back(part.raw);
            else {
                return {};
            }
        }

        peek_distance--;

        t = Type(Path(path));
    }
    while (lexer.peek(peek_distance++).id == ASTERISK) {
        t.pointer_level++;
    }

    peek_distance--;

    // fixme: this is incredibly inefficient
    t.origin = lexer.peek().origin + lexer.peek(peek_distance).origin;
    if (!t.is_primitive())
    t.get_user().origin = t.origin;

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
    // This uses a tiny bit more memory, but saves us from a double free mess
    prefix_parslets.emplace(TRUE, new BoolParselet());
    prefix_parslets.emplace(FALSE, new BoolParselet());

    // Prefix expressions
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

Parser::Parser(std::istream *code, Bucket *bucket, std::vector<std::filesystem::path> paths)
    : lexer(code),
      bucket(bucket),
      search_paths(std::move(paths)) {
    init_parslets();
}

Parser::Parser(const std::filesystem::path &f, Bucket *bucket, std::vector<std::filesystem::path> paths)
    : lexer(f),
      bucket(bucket),
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

Token Parser::expect(TokenType raw) {
    std::string s = to_string(raw);
    Token peek = lexer.peek();
    bucket->iassert(peek.id == raw, peek.origin, "expected a {} found '{}' instead", s, peek.raw);
    return lexer.consume();
}

bool Parser::check_expect(TokenType raw) {
    std::string s = to_string(raw);
    Token peek = lexer.peek();
    bool r = bucket->iassert(peek.id == raw,
                             peek.origin,
                             "expected a {} found '{}' instead",
                             s,
                             peek.raw);
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
    if (!bucket->iassert(prefix_parslets.contains(token.id),
                         token.origin,
                         "expected expression, found '{}'",
                         token.raw))
        return new EmptyExpression(token.origin);
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

    if (token.id == SEMICOLON) {
        lexer.consume();
        return parse_statement();
    } else if (token.id == FUNC) {
        lexer.consume();

        std::optional<Type> member_of = type();

        Token name = Token::name("$$not_assigned");
        if (member_of.has_value()) {
            if (lexer.peek().id == PERIOD) {
                lexer.consume();
                name = expect(NAME);
            } else {
                // We read a type, but it's probably a function name instead
                if (bucket->iassert(
                    !member_of.value().is_primitive() && member_of.value().get_user().get_parts().size() == 1,
                    member_of.value().origin,
                    "Expected function name")) {
                    name = Token::name(member_of.value().get_user().get_parts()[0], member_of.value().origin);
                    member_of = {};
                }
            }
        } else {
            name = expect(NAME);
        }

        std::vector<VariableStatement *> args;
        bool var_arg = false;

        if (!check_expect(PAREN_OPEN)) {
            while (lexer.peek().id != END && lexer.peek().id != PAREN_CLOSE)
                lexer.consume();
        } else {
            if (member_of.has_value() && lexer.peek().raw == "this") {
                Token this_tok = lexer.consume();
                args.push_back(new VariableStatement(this_tok.origin, member_of.value(), this_tok));

                if (lexer.peek().id != PAREN_CLOSE)
                    expect(COMMA);
            }

            while (lexer.peek().id != END && lexer.peek().id != PAREN_CLOSE) {
                if (lexer.peek().id == TRIPLE_PERIOD) {
                    var_arg = true;
                    lexer.consume();
                    break;
                }
                std::optional<Type> maybe_type = type();
                bucket->iassert(maybe_type.has_value(), lexer.peek().origin, "exepcted type name");

                Type arg_type = maybe_type.value_or(Type());

                args.push_back(new VariableStatement(arg_type.origin, arg_type, expect(NAME)));

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
            bucket->iassert(ty.has_value(), lexer.peek().origin, "exepcted type name");
            t = ty.value_or(Type());
        }

        std::vector<Statement *> body = block();

        return new FuncStatement(token.origin, name, t, args, body, var_arg, member_of);
    } else if (token.id == RETURN) {
        lexer.consume();
        Expression *val = nullptr;
        if (lexer.peek().id != SEMICOLON)
            val = parse_expression();
        auto *s = new ReturnStatement(token.origin, val);
        expect(SEMICOLON);
        return s;
    } else if (token.id == IF) {
        lexer.consume();
        auto is = new IfStatement(token.origin, parse_expression(), block());
        if (lexer.peek().id == ELSE) {
            auto es = parse_statement();
            bucket->iassert(es->statement_type == ELSE_STMT,
                            es->origin,
                            "internal: next token is 'else', but parsed statement isn't. report this as a bug");
            is->else_statement = (ElseStatement *) es;
        }
        return is;
    } else if (token.id == ELSE) {
        lexer.consume();
        return new ElseStatement(token.origin, block());
    } else if (token.id == WHILE) {
        lexer.consume();
        return new WhileStatement(token.origin, parse_expression(), block());
    } else if (token.id == BREAK) {
        lexer.consume();
        expect(SEMICOLON);
        return new BreakStatement(token.origin);
    } else if (token.id == CONTINUE) {
        lexer.consume();
        expect(SEMICOLON);
        return new ContinueStatement(token.origin);
    } else if (token.id == CURLY_OPEN) {
        return new ScopeStatement(SCOPE_STMT, token.origin, block());
    } else if (token.id == STRUCT) {
        lexer.consume();

        Token name = expect(NAME);
        expect(CURLY_OPEN);

        std::vector<VariableStatement *> members;

        while (lexer.peek().id != CURLY_CLOSE) {
            std::optional<Type> maybe_type = type();

            bucket->iassert(maybe_type.has_value(), lexer.peek().origin, "expected type name");

            Type member_type = maybe_type.value_or(Type());

            Token member_name = expect(NAME);

            members.push_back(new VariableStatement(member_type.origin, member_type, member_name));
            expect(SEMICOLON);
        }
        lexer.consume();

        return new StructStatement(token.origin, name, members);
    } else if (token.id == IMPORT) {
        lexer.consume();

        auto import_path = find_import();
        std::vector<Statement *> statements;
        if (exists(import_path)) {
            if (std::find(imported.begin(), imported.end(), absolute(import_path)) == imported.end()) {
                imported.push_back(absolute(import_path));
                Parser p(import_path, bucket, search_paths);
                do {
                    statements.push_back(p.parse_statement());
                } while (statements.back());
                statements.pop_back();
            }
        } else {
            bucket->iassert(false,
                            token.origin,
                            "tried to import '{}', but file can't be found",
                            import_path.string());
        }
        expect(SEMICOLON);

        ImportStatement *res;
        for (auto part = --import_path.end(); ; part--) {
            res = new ImportStatement(token.origin, part->stem(), statements);
            statements = {res};

            if (part == import_path.begin())
                break;
        }
        return res;
    } else if (Token peek = lexer.peek(1); peek.id == NAME || peek.id == ASTERISK || peek.id == DOUBLE_COLON) {
        Lexer::State state = lexer.checkpoint();
        if (std::optional<Type> ty = type(); ty.has_value()) {
            Type t = ty.value();

            Token peek = lexer.peek();

            if (peek.id == PAREN_OPEN) {
                lexer.rollback(state);
            } else {
                bucket->iassert(peek.id == NAME,
                                peek.origin,
                                "expected a name found '{}' instead",
                                lexer.peek().raw);
                Token name = lexer.peek();

                if (lexer.peek(1).id != EQUAL) {
                    lexer.consume();
                    expect(SEMICOLON);
                }

                return new VariableStatement(token.origin, t, name);
            }
        }
    }

    Expression *e = parse_expression();
    if (e == reinterpret_cast<Expression *>(-1)) {
        return nullptr;
    } else if (e != nullptr) {
        expect(SEMICOLON);
        return e;
    } else {
        bucket->iassert(false, token.origin, "unexpected token '{}'", token.raw);
        lexer.consume();
        return parse_statement();
    }
}

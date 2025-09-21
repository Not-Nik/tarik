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
        return infix_parslets.at(lexer.peek().id)->get_type();
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

    Token peek = lexer.peek(peek_distance++);
    if (peek.id != TYPE && peek.id != NAME && peek.id != DOUBLE_COLON)
        return {};
    Type t;

    if (peek.id == TYPE) {
        TypeSize size = to_typesize(peek.raw);
        bucket->error(peek.origin, "internal: couldn't find enum member for built-in type")
              ->assert(size != (TypeSize) -1);
        t = Type(size);
    } else {
        LexerRange range = peek.origin;
        std::vector path = {peek.raw};
        if (peek.id == DOUBLE_COLON) {
            path = {""};
            peek_distance--;
        }

        while (lexer.peek(peek_distance++).id == DOUBLE_COLON) {
            Token part = lexer.peek(peek_distance++);
            if (part.id == NAME) {
                path.push_back(part.raw);
                range = range + part.origin;
            } else {
                return {};
            }
        }

        peek_distance--;

        t = Type(Path(path, range));
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
    prefix_parslets.emplace(MACRO_NAME, new MacroNameParselet());
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
    prefix_parslets.emplace(BRACKET_OPEN, new ListParselet());

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
    infix_parslets.emplace(BRACKET_OPEN, new StructInitParselet());

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
    imported.push_back(canonical(f));
    init_parslets();
}

Parser::~Parser() {
    for (auto [type, parselet] : prefix_parslets)
        delete parselet;
    for (auto [type, parselet] : infix_parslets)
        delete parselet;
}

Token Parser::expect(TokenType raw) {
    std::string s = to_string(raw);
    Token peek = lexer.consume();
    if (!bucket->error(peek.origin, "expected a {} found '{}' instead", s, peek.raw)
               ->assert(peek.id == raw)) {
        while (peek.id != raw && peek.id != END) {
            peek = lexer.consume();
        }
    }
    return peek;
}

bool Parser::check_expect(TokenType raw) {
    std::string s = to_string(raw);
    Token peek = lexer.peek();
    bool r = bucket->error(peek.origin,
                           "expected a {} found '{}' instead",
                           s,
                           peek.raw)
                   ->assert(peek.id == raw);
    lexer.consume();
    return r;
}

bool Parser::is_peek(TokenType raw) {
    return lexer.peek().id == raw;
}

ImportPath Parser::find_import() {
    Token next = expect(NAME);

    Path path = Path({next.raw}, next.origin);

    std::filesystem::path import_ = next.raw;

    import_.replace_extension(".tk");

    if (exists(import_)) {
        return {true, path, import_};
    }

    if (exists(next.raw / import_)) {
        return {true, path.create_member(next.raw), next.raw / import_};
    }

    for (const auto &search_path : search_paths) {
        if (exists(search_path / import_)) {
            return {false, path, search_path / next.raw};
        }

        if (exists(search_path / next.raw / import_)) {
            return {false, path.create_member(next.raw), search_path / next.raw / import_};
        }
    }
    return {false, Path({}, {}), {}};
}

Expression *Parser::parse_expression(int precedence) {
    Token token = lexer.peek();
    if (token.id == END)
        return reinterpret_cast<Expression *>(-1);
    if (!bucket->error(token.origin,
                       "expected expression, found '{}'",
                       token.raw)
               ->assert(prefix_parslets.contains(token.id)))
        return new EmptyExpression(token.origin);
    lexer.consume();
    PrefixParselet *prefix = prefix_parslets.at(token.id);

    Expression *left = prefix->parse(this, token);

    while (precedence < get_precedence()) {
        token = lexer.peek();

        InfixParselet *infix = infix_parslets.at(token.id);
        if (infix->can_parse(left))
            left = infix->parse(this, lexer.consume(), left);
        else
            break;
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

        Token name = Token::name("$$not_assigned", token.origin);
        if (member_of.has_value()) {
            if (lexer.peek().id == PERIOD) {
                lexer.consume();
                name = expect(NAME);
            } else {
                // We read a type, but it's probably a function name instead
                if (bucket->error(lexer.peek().origin,
                                  "expected function name")
                          ->assert(
                              !member_of.value().is_primitive() && member_of.value().get_user().get_parts().size() ==
                              1)) {
                    name = Token::name(member_of.value().get_user().get_parts()[0], lexer.peek().origin);
                    member_of = {};
                }
            }
        } else {
            name = expect(NAME);
        }

        std::vector<VariableStatement *> args;

        if (!check_expect(PAREN_OPEN)) {
            while (lexer.peek().id != END && lexer.peek().id != PAREN_CLOSE)
                lexer.consume();
        } else {
            if (member_of.has_value() &&
                (lexer.peek().raw == "this" || (lexer.peek().id == ASTERISK && lexer.peek(1).raw == "this"))) {
                Token this_tok = lexer.consume();
                Type this_type = member_of.value();
                if (this_tok.id == ASTERISK) {
                    this_tok = lexer.consume();
                    this_type = this_type.get_pointer_to();
                }

                args.push_back(new VariableStatement(this_type, this_tok));

                if (lexer.peek().id != PAREN_CLOSE)
                    expect(COMMA);
            }

            while (lexer.peek().id != END && lexer.peek().id != PAREN_CLOSE) {
                std::optional<Type> maybe_type = type();
                bucket->error(lexer.peek().origin, "exepcted type name")
                      ->assert(maybe_type.has_value());

                Type arg_type = maybe_type.value_or(Type());

                args.push_back(new VariableStatement(arg_type, expect(NAME)));

                if (lexer.peek().id != PAREN_CLOSE)
                    expect(COMMA);
            }
        }
        LexerRange origin = token.origin + expect(PAREN_CLOSE).origin;
        Type t;
        if (lexer.peek().id != CURLY_OPEN) {
            std::optional<Type> ty = type();
            bucket->error(lexer.peek().origin, "exepcted type name")
                  ->assert(ty.has_value());
            t = ty.value_or(Type());
        }

        std::vector<Statement *> body = block();

        return new FuncStatement(origin, name, t, args, body, member_of);
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
        auto *is = new IfStatement(token.origin, parse_expression(), block());
        if (lexer.peek().id == ELSE) {
            Statement *es = parse_statement();
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

            if (!bucket->error(lexer.peek().origin, "expected type name")
                       ->assert(maybe_type.has_value())) {
                while (lexer.peek().id != CURLY_CLOSE && lexer.peek().id != END) {
                    lexer.consume();
                }
                break;
            }

            Type member_type = maybe_type.value_or(Type());

            Token member_name = expect(NAME);

            members.push_back(new VariableStatement(member_type, member_name));
            expect(SEMICOLON);
        }
        expect(CURLY_CLOSE);

        return new StructStatement(token.origin, name, members);
    } else if (token.id == IMPORT) {
        lexer.consume();

        ImportPath imp = find_import();
        std::vector<Statement *> statements;
        if (exists(imp.file)) {
            if (std::find(imported.begin(), imported.end(), canonical(imp.file)) == imported.end()) {
                Parser p(imp.file, bucket, search_paths);
                std::filesystem::path cwd = std::filesystem::current_path();
                std::filesystem::current_path(canonical(imp.file).parent_path());
                do {
                    statements.push_back(p.parse_statement());
                } while (statements.back());
                statements.pop_back();
                std::filesystem::current_path(cwd);
            }
        } else {
            bucket->error(token.origin,
                          "tried to import '{}', but file can't be found",
                          imp.file.string());
        }
        expect(SEMICOLON);

        return new ImportStatement(token.origin, imp.path, imp.local, statements);
    } else if (Token peek = lexer.peek(1); peek.id == NAME || peek.id == ASTERISK || peek.id == DOUBLE_COLON) {
        Lexer::State state = lexer.checkpoint();
        if (std::optional<Type> ty = type(); ty.has_value()) {
            Type t = ty.value();

            Token peek = lexer.peek();

            if (peek.id != NAME) {
                lexer.rollback(state);
            } else {
                Token name = lexer.peek();

                if (lexer.peek(1).id != EQUAL) {
                    lexer.consume();
                    expect(SEMICOLON);
                }

                return new VariableStatement(t, name);
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
        bucket->error(token.origin, "unexpected token '{}'", token.raw);
        lexer.consume();
        return parse_statement();
    }
}

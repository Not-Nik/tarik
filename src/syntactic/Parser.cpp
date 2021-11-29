// tarik (c) Nikolas Wipper 2020

#include "Parser.h"

#include "util/Util.h"
#include "expressions/Parslets.h"

#include <comperr.h>

#include <utility>

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
    while (!lexer.peek().raw.empty() && lexer.peek().id != CURLY_CLOSE) {
        Statement *statement = parse_statement();
        res.push_back(statement);
    }
    expect(CURLY_CLOSE);
    return res;
}

Type Parser::type() {
    auto peek = lexer.peek();
    iassert(peek.id == TYPE or (peek.id == NAME and has_struct_with_name(peek.raw)), "expected type name");
    Type t;

    if (peek.id == TYPE) {
        std::string type_name;
        for (auto c: peek.raw)
            type_name.push_back((char) toupper(c));
        TypeSize size = to_typesize(type_name);
        iassert(size != (TypeSize) -1, "internal: couldn't find enum member for built-in type");
        t.type.size = size;
    } else {
        StructStatement *structure;
        for (StructStatement *st: structures) {
            if (st->name == peek.raw) {
                structure = st;
                break;
            }
        }
        // this is unreachable?
        iassert(structure, "undefined structure %s", peek.raw.c_str());
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
    infix_parslets.emplace(PERIOD, new MemberAccessParselet());

    // Call
    infix_parslets.emplace(PAREN_OPEN, new CallParselet());

    // Assign expressions
    infix_parslets.emplace(EQUAL, new AssignParselet());
}

Parser::Parser(std::istream *code, bool dry)
    : lexer(code), dry_parsing(dry) {
    init_parslets();
}

Parser::Parser(const std::filesystem::path &f, bool dry)
    : lexer(f), dry_parsing(dry) {
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
    iassert(lexer.peek().id == raw, "expected a '%s' found '%s' instead", s.c_str(), lexer.peek().raw.c_str());
    return lexer.consume();
}

bool Parser::is_peek(TokenType raw) {
    return lexer.peek().id == raw;
}

bool Parser::check_expect(TokenType raw) {
    std::string s = to_string(raw);
    bool r = iassert(lexer.peek().id == raw, "expected a '%s' found '%s' instead", s.c_str(), lexer.peek().raw.c_str());
    lexer.consume();
    return r;
}

StructStatement *Parser::register_struct(StructStatement *struct_) {
    structures.push_back(struct_);
    return struct_;
}

bool Parser::has_struct_with_name(const std::string &name) {
    for (auto str : structures) {
        if (str->name == name) return true;
    }
    return false;
}

Expression *Parser::parse_expression(int precedence) {
    Token token = lexer.consume();
    if (token.raw.empty()) return reinterpret_cast<Expression *>(-1);
    if (!iassert(prefix_parslets.count(token.id) > 0, "unexpected token '%s'", token.raw.c_str()))
        return nullptr;
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
    if (token.id == END) return nullptr;

    if (token.id == FUNC) {
        lexer.consume();
        Token name_tok = expect(NAME);
        std::string name = name_tok.raw;

        std::vector<VariableStatement *> args;
        bool var_arg = false;

        if (!check_expect(PAREN_OPEN))
            while (!lexer.peek().raw.empty() && lexer.peek().id != PAREN_CLOSE) { lexer.consume(); }
        else
            while (!lexer.peek().raw.empty() && lexer.peek().id != PAREN_CLOSE) {
                if (lexer.peek().id == TRIPLE_PERIOD) {
                    var_arg = true;
                    lexer.consume();
                    break;
                }
                args.push_back(new VariableStatement(where(), type(), expect(NAME).raw));

                if (lexer.peek().id != PAREN_CLOSE)
                    expect(COMMA);
            }
        expect(PAREN_CLOSE);
        Type t;
        if (lexer.peek().id == CURLY_OPEN || lexer.peek().id == SEMICOLON) t = Type(VOID);
        else t = type();

        if (lexer.peek().id == SEMICOLON) {
            lexer.consume();
            return new FuncDeclareStatement(name_tok.where, name, t, args, var_arg);
        }

        std::vector<Statement *> body = block();
        if (dry_parsing) {
            for (auto s : body) delete s;
            return new FuncDeclareStatement(name_tok.where, name, t, args, var_arg, false);
        } else {
            return new FuncStatement(name_tok.where, name, t, args, body, var_arg);
        }
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
            iassert(es->statement_type == ELSE_STMT, "internal: next token is 'else', but parsed statement isn't. report this as a bug");
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
    } else if (token.id == STRUCT) {
        lexer.consume();

        iassert(is_peek(NAME), "expected an identifier found '%s' instead", lexer.peek().raw.c_str());
        std::string name = lexer.consume().raw;

        expect(CURLY_OPEN);

        std::vector<VariableStatement *> members;

        while (lexer.peek().id != CURLY_CLOSE) {
            Type member_type = type();

            iassert(is_peek(NAME), "expected an identifier found '%s' instead", lexer.peek().raw.c_str());
            std::string member_name = lexer.consume().raw;

            members.push_back(new VariableStatement(where(), member_type, member_name));
            expect(SEMICOLON);
        }
        lexer.consume();

        return register_struct(new StructStatement(token.where, name, members));
    } else if (token.id == IMPORT) {
        lexer.consume();

        Token import = expect(STRING);
        std::vector<Statement *> statements;
        if (import.id == STRING) {
            std::filesystem::path path = std::filesystem::absolute(import.raw);
            if (std::find(imported.begin(), imported.end(), path) == imported.end()) {
                imported.push_back(path);
                if (iassert(exists(path), "tried to import '%s', but file can't be found", import.raw.c_str())) {
                    Parser p(path, true);
                    do {
                        statements.push_back(p.parse_statement());
                    } while (statements.back());
                    statements.pop_back();
                }
            }
        }
        expect(SEMICOLON);

        return new ImportStatement(token.where, statements);
    } else if (token.id == TYPE or (token.id == NAME and has_struct_with_name(token.raw))) {
        Type t = type();

        iassert(is_peek(NAME), "expected an identifier found '%s' instead", lexer.peek().raw.c_str());
        std::string name = lexer.peek().raw;

        if (lexer.peek(1).id != EQUAL) {
            lexer.consume();
            expect(SEMICOLON);
        }

        return new VariableStatement(where(), t, name);
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

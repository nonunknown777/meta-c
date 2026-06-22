#include "parser.h"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace brick {

static bool is_auto_semicolon(TokenType t) {
    switch (t) {
        case TokenType::IDENTIFIER:
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::CHAR_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE:
        case TokenType::NULL_:
        case TokenType::RPAREN:
        case TokenType::RBRACKET:
        case TokenType::RETURN:
            return true;
        default:
            return false;
    }
}

static std::vector<Token> insert_auto_semicolons(const std::vector<Token>& input) {
    std::vector<Token> output;
    if (input.empty()) return output;

    output.push_back(input[0]);
    for (size_t i = 1; i < input.size(); i++) {
        const Token& prev = input[i - 1];
        const Token& curr = input[i];

        bool has_newline = prev.location.line < curr.location.line;

        if (has_newline && is_auto_semicolon(prev.type)) {
            output.emplace_back(TokenType::SEMICOLON, ";", prev.location);
        }
        output.push_back(curr);
    }
    return output;
}

class Parser {
public:
    Parser(const std::vector<Token>& tokens)
        : tokens(insert_auto_semicolons(tokens)), pos(0) {}

    ParseResult parse_all() {
        ParseResult result;
        try {
            result.ast = program();
        } catch (const std::exception& e) {
            result.errors.push_back(e.what());
        }
        return result;
    }

private:
    std::vector<Token> tokens;
    size_t pos;

    const Token& peek() const {
        return tokens[pos];
    }

    const Token& advance() {
        return tokens[pos++];
    }

    bool match(TokenType type) {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    }

    Token expect(TokenType type, const std::string& msg) {
        if (peek().type != type) {
            throw std::runtime_error(msg + " at " +
                std::to_string(peek().location.line) + ":" +
                std::to_string(peek().location.col));
        }
        return advance();
    }

    bool is_type_keyword(TokenType t) const {
        switch (t) {
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::BOOL:
            case TokenType::CHAR:
            case TokenType::STRING:
            case TokenType::VOID:
            case TokenType::U8:
            case TokenType::U16:
            case TokenType::U32:
            case TokenType::U64:
            case TokenType::I8:
            case TokenType::I16:
            case TokenType::I32:
            case TokenType::I64:
            case TokenType::F32:
            case TokenType::F64:
            case TokenType::USIZE:
            case TokenType::ISIZE:
            case TokenType::BYTE:
                return true;
            default:
                return false;
        }
    }

    bool is_type_start() const {
        return is_type_keyword(peek().type) || peek().type == TokenType::IDENTIFIER;
    }

    std::string parse_type_name() {
        if (is_type_keyword(peek().type)) {
            return advance().lexeme;
        }
        return expect(TokenType::IDENTIFIER, "expected type name").lexeme;
    }

    // ─── Top-level ───
    // ─── Top-level (topo) ───

    std::unique_ptr<ProgramNode> program() {
        auto prog = std::make_unique<ProgramNode>(peek().location);
        while (peek().type != TokenType::EOF_) {
            prog->declarations.push_back(declaration());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        return prog;
    }

    std::unique_ptr<ASTNode> declaration() {
        switch (peek().type) {
            case TokenType::PACKAGE: return package_decl();
            case TokenType::USING:   return using_decl();
            case TokenType::PRIVATE: return private_decl();
            case TokenType::STRUCT:  return struct_decl();
            case TokenType::INTERFACE: return interface_decl();
            case TokenType::BLOCK:   return block_decl_or_scope();
            case TokenType::FN:      return func_decl();
            default:
                throw std::runtime_error("unexpected token '" + peek().lexeme + "'");
        }
    }

    std::unique_ptr<ASTNode> package_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::vector<std::string> parts;
        parts.push_back(expect(TokenType::IDENTIFIER, "expected package name").lexeme);
        while (match(TokenType::DOT)) {
            parts.push_back(expect(TokenType::IDENTIFIER, "expected sub-package name").lexeme);
        }
        return std::make_unique<PackageDecl>(std::move(parts), loc);
    }

    std::unique_ptr<ASTNode> using_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::vector<std::string> parts;
        parts.push_back(expect(TokenType::IDENTIFIER, "expected package name").lexeme);
        while (match(TokenType::DOT)) {
            parts.push_back(expect(TokenType::IDENTIFIER, "expected sub-package name").lexeme);
        }
        return std::make_unique<UsingDecl>(std::move(parts), loc);
    }

    std::unique_ptr<ASTNode> private_decl() {
        advance();
        auto decl = declaration();
        auto set_private = [](ASTNode* node) {
            if (auto* sd = dynamic_cast<StructDecl*>(node)) {
                sd->is_private = true;
            } else if (auto* fd = dynamic_cast<FuncDecl*>(node)) {
                fd->is_private = true;
            } else if (auto* fld = dynamic_cast<FieldDecl*>(node)) {
                fld->is_private = true;
            }
        };
        set_private(decl.get());
        return decl;
    }

    std::unique_ptr<ASTNode> struct_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::string name = expect(TokenType::IDENTIFIER, "expected struct name").lexeme;
        auto sd = std::make_unique<StructDecl>(name, loc);

        if (match(TokenType::EXTENDS)) {
            sd->extends = expect(TokenType::IDENTIFIER, "expected base struct name").lexeme;
            while (match(TokenType::COMMA)) {
                sd->interfaces.push_back(
                    expect(TokenType::IDENTIFIER, "expected interface name").lexeme);
            }
        }

        expect(TokenType::LBRACE, "expected '{' after struct name");

        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }

            if (peek().type == TokenType::FN) {
                sd->methods.push_back(func_decl());
            } else {
                auto field = field_decl();
                if (field) sd->fields.push_back(std::move(field));
            }
            if (peek().type == TokenType::SEMICOLON) advance();
        }

        expect(TokenType::RBRACE, "expected '}' after struct body");
        return sd;
    }

    std::unique_ptr<ASTNode> field_decl() {
        SourceLocation loc = peek().location;
        std::string type_name = parse_type_name();

        if (match(TokenType::LBRACKET)) {
            type_name += "[]";
            if (peek().type == TokenType::INT_LITERAL) {
                type_name = type_name.substr(0, type_name.size() - 1) + advance().lexeme + "]";
            }
            expect(TokenType::RBRACKET, "expected ']' after array size");
        }

        std::string name = expect(TokenType::IDENTIFIER, "expected field name").lexeme;
        return std::make_unique<FieldDecl>(type_name, name, loc);
    }

    std::unique_ptr<ASTNode> interface_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::string name = expect(TokenType::IDENTIFIER, "expected interface name").lexeme;
        auto id = std::make_unique<InterfaceDecl>(name, loc);

        expect(TokenType::LBRACE, "expected '{' after interface name");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }

            expect(TokenType::FN, "expected 'fn' in interface method");
            auto method = interface_method_decl();
            id->methods.push_back(std::move(method));

            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' after interface body");
        return id;
    }

    std::unique_ptr<ASTNode> interface_method_decl() {
        SourceLocation loc = peek().location;
        std::string name = expect(TokenType::IDENTIFIER, "expected method name").lexeme;
        auto fd = std::make_unique<FuncDecl>(name, loc);

        expect(TokenType::LPAREN, "expected '(' after method name");
        if (peek().type != TokenType::RPAREN) {
            fd->params = param_list();
        }
        expect(TokenType::RPAREN, "expected ')' after parameters");

        if (match(TokenType::ARROW)) {
            fd->return_type = parse_type_name();
        }

        return fd;
    }

    std::unique_ptr<ASTNode> func_decl() {
        SourceLocation loc = peek().location;
        advance();

        std::string name = expect(TokenType::IDENTIFIER, "expected function name").lexeme;
        auto fd = std::make_unique<FuncDecl>(name, loc);

        expect(TokenType::LPAREN, "expected '(' after function name");
        if (peek().type != TokenType::RPAREN) {
            fd->params = param_list();
        }
        expect(TokenType::RPAREN, "expected ')' after parameters");

        if (match(TokenType::ARROW)) {
            fd->return_type = parse_type_name();
        }

        fd->body = block_stmt();
        return fd;
    }

    std::vector<std::unique_ptr<ASTNode>> param_list() {
        std::vector<std::unique_ptr<ASTNode>> params;
        do {
            std::string type_name = parse_type_name();
            std::string name = expect(TokenType::IDENTIFIER, "expected parameter name").lexeme;
            params.push_back(std::make_unique<ParamDecl>(type_name, name, peek().location));
        } while (match(TokenType::COMMA));
        return params;
    }

    std::unique_ptr<ASTNode> block_decl_or_scope() {
        SourceLocation loc = peek().location;
        advance();
        std::string name = expect(TokenType::IDENTIFIER, "expected block name").lexeme;

        if (match(TokenType::ASSIGN)) {
            int64_t size = 0;
            if (peek().type == TokenType::INT_LITERAL) {
                size = std::stoll(advance().lexeme);
            }
            std::string unit = "B";
            if (peek().type == TokenType::IDENTIFIER) {
                unit = advance().lexeme;
            }
            return std::make_unique<BlockDecl>(name, size, unit, loc);
        }

        if (match(TokenType::LBRACE)) {
            auto bs = std::make_unique<BlockScope>(name, loc);
            while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
                if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
                bs->body.push_back(statement());
                if (peek().type == TokenType::SEMICOLON) advance();
            }
            expect(TokenType::RBRACE, "expected '}' after block scope");
            return bs;
        }

        return std::make_unique<BlockScope>(name, loc);
    }

    // ─── Statements ───
    // ─── Statements (comandos) ───

    std::unique_ptr<BlockStmt> block_stmt() {
        auto bs = std::make_unique<BlockStmt>(peek().location);
        expect(TokenType::LBRACE, "expected '{' for block");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            bs->statements.push_back(statement());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' to close block");
        return bs;
    }

    std::unique_ptr<ASTNode> statement() {
        switch (peek().type) {
            case TokenType::IF:     return if_stmt();
            case TokenType::WHILE:  return while_stmt();
            case TokenType::FOR:    return for_stmt();
            case TokenType::RETURN: return return_stmt();
            case TokenType::BLOCK:  return block_decl_or_scope();
            case TokenType::LBRACE: return block_stmt();
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::BOOL:
            case TokenType::CHAR:
            case TokenType::STRING:
            case TokenType::VOID:
            case TokenType::U8:
            case TokenType::U16:
            case TokenType::U32:
            case TokenType::U64:
            case TokenType::I8:
            case TokenType::I16:
            case TokenType::I32:
            case TokenType::I64:
            case TokenType::F32:
            case TokenType::F64:
            case TokenType::USIZE:
            case TokenType::ISIZE:
            case TokenType::BYTE:
                return var_decl();
            default:
                if (peek().type == TokenType::IDENTIFIER && pos + 1 < tokens.size()) {
                    if (tokens[pos + 1].type == TokenType::IDENTIFIER ||
                        tokens[pos + 1].type == TokenType::LBRACKET) {
                        return var_decl();
                    }
                }
                return expr_stmt();
        }
    }

    std::unique_ptr<ASTNode> var_decl() {
        SourceLocation loc = peek().location;
        std::string type_name = parse_type_name();

        if (match(TokenType::LBRACKET)) {
            type_name += "[]";
            if (peek().type == TokenType::INT_LITERAL) {
                type_name = type_name.substr(0, type_name.size() - 1) + advance().lexeme + "]";
            }
            expect(TokenType::RBRACKET, "expected ']' after array size");
        }

        std::string name = expect(TokenType::IDENTIFIER, "expected variable name").lexeme;

        if (match(TokenType::ASSIGN)) {
            auto init = expression();
            if (peek().type == TokenType::AT) {
                advance();
                std::string block_name =
                    expect(TokenType::IDENTIFIER, "expected block name after '@'").lexeme;
                init = std::make_unique<AllocInline>(std::move(init), block_name, init->location);
            }
            auto assign = std::make_unique<Assignment>(TokenType::ASSIGN, loc);
            auto target = std::make_unique<IdentExpr>(name, loc);
            target->declared_type = type_name;
            assign->target = std::move(target);
            assign->value = std::move(init);
            return std::make_unique<ExprStmt>(std::move(assign), loc);
        }

        if (peek().type == TokenType::AT) {
            advance();
            expect(TokenType::IDENTIFIER, "expected block name after '@'");
        }

        auto target = std::make_unique<IdentExpr>(name, loc);
        target->declared_type = type_name;
        return std::make_unique<ExprStmt>(std::move(target), loc);
    }

    // if cond { } ou if(cond) { }
    std::unique_ptr<ASTNode> if_stmt() {
        advance();
        auto is = std::make_unique<IfStmt>(peek().location);
        if (peek().type == TokenType::LPAREN) {
            advance();
            is->condition = expression();
            expect(TokenType::RPAREN, "expected ')' after condition");
        } else {
            is->condition = expression();
        }
        is->then_branch = block_stmt();
        if (match(TokenType::ELSE)) {
            is->else_branch = statement();
        }
        return is;
    }

    std::unique_ptr<ASTNode> while_stmt() {
        advance();
        auto ws = std::make_unique<WhileStmt>(peek().location);
        if (peek().type == TokenType::LPAREN) {
            advance();
            ws->condition = expression();
            expect(TokenType::RPAREN, "expected ')' after condition");
        } else {
            ws->condition = expression();
        }
        ws->body = block_stmt();
        return ws;
    }

    std::unique_ptr<ASTNode> for_stmt() {
        advance();
        auto fs = std::make_unique<ForStmt>(peek().location);
        expect(TokenType::LPAREN, "expected '(' after for");

        if (is_type_start()) {
            fs->init = var_decl();
        } else if (peek().type != TokenType::SEMICOLON) {
            fs->init = expr_stmt();
        }
        expect(TokenType::SEMICOLON, "expected ';' after for init");

        if (peek().type != TokenType::SEMICOLON) {
            fs->condition = expression();
        }
        expect(TokenType::SEMICOLON, "expected ';' after for condition");

        if (peek().type != TokenType::RPAREN) {
            fs->increment = expression();
        }
        expect(TokenType::RPAREN, "expected ')' after for clauses");

        fs->body = block_stmt();
        return fs;
    }

    std::unique_ptr<ASTNode> return_stmt() {
        SourceLocation loc = peek().location;
        advance();
        auto rs = std::make_unique<ReturnStmt>(loc);
        if (peek().type != TokenType::SEMICOLON && peek().type != TokenType::RBRACE &&
            peek().type != TokenType::EOF_) {
            rs->value = expression();
        }
        if (peek().type == TokenType::SEMICOLON) advance();
        return rs;
    }

    std::unique_ptr<ASTNode> expr_stmt() {
        auto expr = expression();
        if (peek().type == TokenType::SEMICOLON) advance();
        return std::make_unique<ExprStmt>(std::move(expr), expr->location);
    }

    // ─── Expressions ───
    // ─── Expressions (expressoes) ───

    std::unique_ptr<ASTNode> expression() {
        return assignment();
    }

    std::unique_ptr<ASTNode> assignment() {
        auto left = logical_or();
        if (peek().type == TokenType::ASSIGN ||
            peek().type == TokenType::PLUS_ASSIGN ||
            peek().type == TokenType::MINUS_ASSIGN ||
            peek().type == TokenType::STAR_ASSIGN ||
            peek().type == TokenType::SLASH_ASSIGN) {
            TokenType op = advance().type;
            auto value = assignment();
            auto assign = std::make_unique<Assignment>(op, left->location);
            assign->target = std::move(left);
            assign->value = std::move(value);
            return assign;
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_or() {
        auto left = logical_and();
        while (peek().type == TokenType::OR) {
            TokenType op = advance().type;
            auto right = logical_and();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_and() {
        auto left = equality();
        while (peek().type == TokenType::AND) {
            TokenType op = advance().type;
            auto right = equality();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> equality() {
        auto left = comparison();
        while (peek().type == TokenType::EQ || peek().type == TokenType::NEQ) {
            TokenType op = advance().type;
            auto right = comparison();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> comparison() {
        auto left = term();
        while (peek().type == TokenType::LT || peek().type == TokenType::GT ||
               peek().type == TokenType::LEQ || peek().type == TokenType::GEQ) {
            TokenType op = advance().type;
            auto right = term();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> term() {
        auto left = factor();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            TokenType op = advance().type;
            auto right = factor();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> factor() {
        auto left = unary();
        while (peek().type == TokenType::STAR || peek().type == TokenType::SLASH) {
            TokenType op = advance().type;
            auto right = unary();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> unary() {
        if (peek().type == TokenType::NOT || peek().type == TokenType::MINUS) {
            TokenType op = advance().type;
            auto operand = unary();
            return std::make_unique<UnaryOp>(op, std::move(operand), operand->location);
        }
        return call();
    }

    std::unique_ptr<ASTNode> call() {
        auto expr = primary();

        while (true) {
            if (peek().type == TokenType::LPAREN) {
                advance();
                auto call = std::make_unique<CallExpr>(expr->location);
                call->callee = std::move(expr);
                if (peek().type != TokenType::RPAREN) {
                    do {
                        call->arguments.push_back(expression());
                    } while (match(TokenType::COMMA));
                }
                expect(TokenType::RPAREN, "expected ')' after arguments");
                expr = std::move(call);
            } else if (peek().type == TokenType::DOT) {
                advance();
                std::string member;
                if (peek().type == TokenType::RESET) {
                    member = advance().lexeme;
                } else {
                    member = expect(TokenType::IDENTIFIER, "expected member name").lexeme;
                }
                expr = std::make_unique<MemberExpr>(std::move(expr), member, expr->location);
            } else if (peek().type == TokenType::LBRACKET) {
                advance();
                auto index = std::make_unique<IndexExpr>(expr->location);
                index->array = std::move(expr);
                index->index = expression();
                expect(TokenType::RBRACKET, "expected ']' after index");
                expr = std::move(index);
            } else if (peek().type == TokenType::AT) {
                advance();
                std::string block_name =
                    expect(TokenType::IDENTIFIER, "expected block name after '@'").lexeme;
                expr = std::make_unique<AllocInline>(std::move(expr), block_name, expr->location);
            } else {
                break;
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> primary() {
        SourceLocation loc = peek().location;

        switch (peek().type) {
            case TokenType::INT_LITERAL: {
                Token t = advance();
                int64_t val = std::stoll(t.lexeme);
                auto lit = std::make_unique<IntLiteral>(val, loc);
                lit->literal_type = t.literal_type;
                return lit;
            }
            case TokenType::FLOAT_LITERAL: {
                Token t = advance();
                double val = std::stod(t.lexeme);
                auto lit = std::make_unique<FloatLiteral>(val, loc);
                lit->literal_type = t.literal_type;
                return lit;
            }
            case TokenType::STRING_LITERAL:
                return std::make_unique<StringLiteral>(advance().lexeme, loc);
            case TokenType::CHAR_LITERAL:
                return std::make_unique<CharLiteral>(advance().lexeme[0], loc);
            case TokenType::TRUE:
                advance();
                return std::make_unique<BoolLiteral>(true, loc);
            case TokenType::FALSE:
                advance();
                return std::make_unique<BoolLiteral>(false, loc);
            case TokenType::NULL_:
                advance();
                return std::make_unique<NullLiteral>(loc);
            case TokenType::ERROR: {
                advance();
                expect(TokenType::LPAREN, "expected '(' after error");
                auto msg = expect(TokenType::STRING_LITERAL, "expected error message").lexeme;
                expect(TokenType::RPAREN, "expected ')' after error message");
                auto call = std::make_unique<CallExpr>(loc);
                call->callee = std::make_unique<IdentExpr>("error", loc);
                call->arguments.push_back(std::make_unique<StringLiteral>(msg, loc));
                return call;
            }
            case TokenType::IDENTIFIER:
                return std::make_unique<IdentExpr>(advance().lexeme, loc);
            case TokenType::LPAREN: {
                advance();
                auto expr = expression();
                expect(TokenType::RPAREN, "expected ')' after expression");
                return expr;
            }
            default:
                throw std::runtime_error("unexpected token '" + peek().lexeme + "' in expression");
        }
    }
};

ParseResult parse(const std::vector<Token>& tokens) {
    Parser parser(tokens);
    return parser.parse_all();
}

} // namespace brick
  // namespace brick

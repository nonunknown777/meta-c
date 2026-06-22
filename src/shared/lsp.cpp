#include "lsp.h"
#include <sstream>
#include <cctype>

namespace brick {

static std::string escape_json(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

static std::string token_type_name(TokenType t) {
    switch (t) {
        case TokenType::PACKAGE: return "PACKAGE";
        case TokenType::USING: return "USING";
        case TokenType::PRIVATE: return "PRIVATE";
        case TokenType::PUBLIC: return "PUBLIC";
        case TokenType::STRUCT: return "STRUCT";
        case TokenType::EXTENDS: return "EXTENDS";
        case TokenType::INTERFACE: return "INTERFACE";
        case TokenType::FN: return "FN";
        case TokenType::RETURN: return "RETURN";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FOR: return "FOR";
        case TokenType::BLOCK: return "BLOCK";
        case TokenType::RESET: return "RESET";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::NULL_: return "NULL";
        case TokenType::ERROR: return "ERROR";
        case TokenType::INT: return "INT";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::BOOL: return "BOOL";
        case TokenType::CHAR: return "CHAR";
        case TokenType::STRING: return "STRING";
        case TokenType::VOID: return "VOID";
        case TokenType::U8: return "U8";
        case TokenType::U16: return "U16";
        case TokenType::U32: return "U32";
        case TokenType::U64: return "U64";
        case TokenType::I8: return "I8";
        case TokenType::I16: return "I16";
        case TokenType::I32: return "I32";
        case TokenType::I64: return "I64";
        case TokenType::F32: return "F32";
        case TokenType::F64: return "F64";
        case TokenType::USIZE: return "USIZE";
        case TokenType::ISIZE: return "ISIZE";
        case TokenType::BYTE: return "BYTE";
        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TokenType::STAR_ASSIGN: return "STAR_ASSIGN";
        case TokenType::SLASH_ASSIGN: return "SLASH_ASSIGN";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LEQ: return "LEQ";
        case TokenType::GEQ: return "GEQ";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::BIT_AND: return "BIT_AND";
        case TokenType::BIT_OR: return "BIT_OR";
        case TokenType::BIT_XOR: return "BIT_XOR";
        case TokenType::BIT_NOT: return "BIT_NOT";
        case TokenType::LSHIFT: return "LSHIFT";
        case TokenType::RSHIFT: return "RSHIFT";
        case TokenType::DOT: return "DOT";
        case TokenType::ARROW: return "ARROW";
        case TokenType::AT: return "AT";
        case TokenType::PIPE: return "PIPE";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::EOF_: return "EOF";
    }
    return "UNKNOWN";
}

void collect_symbols(ASTNode* node, std::vector<LspSymbol>& symbols) {
    if (!node) return;

    switch (node->type) {
        case ASTNodeType::STRUCT_DECL: {
            auto* sd = static_cast<StructDecl*>(node);
            symbols.push_back({
                sd->name, "struct", "struct",
                sd->location
            });
            for (auto& f : sd->fields) collect_symbols(f.get(), symbols);
            for (auto& m : sd->methods) collect_symbols(m.get(), symbols);
            return;
        }
        case ASTNodeType::INTERFACE_DECL: {
            auto* id = static_cast<InterfaceDecl*>(node);
            symbols.push_back({
                id->name, "interface", "interface",
                id->location
            });
            for (auto& m : id->methods) collect_symbols(m.get(), symbols);
            return;
        }
        case ASTNodeType::FUNC_DECL: {
            auto* fd = static_cast<FuncDecl*>(node);
            std::string kind = fd->is_constructor ? "constructor" : "function";
            std::string type_name = fd->return_type.empty() ? "void" : fd->return_type;
            symbols.push_back({
                fd->name, kind, type_name,
                fd->location
            });
            for (auto& p : fd->params) collect_symbols(p.get(), symbols);
            return;
        }
        case ASTNodeType::FIELD_DECL: {
            auto* fld = static_cast<FieldDecl*>(node);
            symbols.push_back({
                fld->name, "field", fld->type_name,
                fld->location
            });
            return;
        }
        case ASTNodeType::PARAM_DECL: {
            auto* pd = static_cast<ParamDecl*>(node);
            symbols.push_back({
                pd->name, "param", pd->type_name,
                pd->location
            });
            return;
        }
        case ASTNodeType::BLOCK_DECL: {
            auto* bd = static_cast<BlockDecl*>(node);
            symbols.push_back({
                bd->name, "block", "block",
                bd->location
            });
            return;
        }
        case ASTNodeType::BLOCK_SCOPE: {
            auto* bs = static_cast<BlockScope*>(node);
            for (auto& s : bs->body) collect_symbols(s.get(), symbols);
            return;
        }
        case ASTNodeType::PROGRAM: {
            auto* pn = static_cast<ProgramNode*>(node);
            for (auto& d : pn->declarations) collect_symbols(d.get(), symbols);
            return;
        }
        case ASTNodeType::PACKAGE_DECL:
        case ASTNodeType::USING_DECL:
        case ASTNodeType::BLOCK_STMT:
        case ASTNodeType::IF_STMT:
        case ASTNodeType::WHILE_STMT:
        case ASTNodeType::FOR_STMT:
        case ASTNodeType::RETURN_STMT:
        case ASTNodeType::EXPR_STMT:
        case ASTNodeType::ALLOC_INLINE:
        case ASTNodeType::RESET_EXPR:
        case ASTNodeType::INT_LITERAL:
        case ASTNodeType::FLOAT_LITERAL:
        case ASTNodeType::STRING_LITERAL:
        case ASTNodeType::BOOL_LITERAL:
        case ASTNodeType::CHAR_LITERAL:
        case ASTNodeType::NULL_LITERAL:
        case ASTNodeType::IDENT_EXPR:
        case ASTNodeType::CALL_EXPR:
        case ASTNodeType::MEMBER_EXPR:
        case ASTNodeType::INDEX_EXPR:
        case ASTNodeType::BINARY_OP:
        case ASTNodeType::UNARY_OP:
        case ASTNodeType::ASSIGNMENT:
            break;
    }
}

LspError parse_error_string(const std::string& err_str, const std::string& default_file) {
    LspError err;
    err.location.file = default_file;
    err.location.line = 0;
    err.location.col = 0;
    err.message = err_str;
    err.severity = 1;

    // Try to parse "file:line: message" format
    // Tenta analisar formato "file:line: message"
    auto first_colon = err_str.find(':');
    if (first_colon == std::string::npos) return err;

    auto second_colon = err_str.find(':', first_colon + 1);
    if (second_colon == std::string::npos) return err;

    std::string file_part = err_str.substr(0, first_colon);
    std::string line_part = err_str.substr(first_colon + 1, second_colon - first_colon - 1);
    std::string msg_part = err_str.substr(second_colon + 1);

    // Trim leading space from message
    // Remove espaco inicial da mensagem
    if (!msg_part.empty() && msg_part[0] == ' ') msg_part = msg_part.substr(1);

    err.location.file = file_part;
    err.message = msg_part;

    try {
        err.location.line = std::stoi(line_part);
    } catch (...) {}

    return err;
}

std::string emit_lsp_json(const LspOutput& output) {
    std::ostringstream json;
    json << "{\n";

    // Tokens
    // Tokens
    json << "  \"tokens\": [\n";
    for (size_t i = 0; i < output.tokens.size(); i++) {
        const auto& tok = output.tokens[i];
        json << "    {\n";
        json << "      \"type\": \"" << escape_json(token_type_name(tok.type)) << "\",\n";
        json << "      \"lexeme\": \"" << escape_json(tok.lexeme) << "\",\n";
        json << "      \"line\": " << tok.location.line << ",\n";
        json << "      \"col\": " << tok.location.col << ",\n";
        json << "      \"file\": \"" << escape_json(tok.location.file) << "\"\n";
        json << "    }";
        if (i < output.tokens.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";

    // Symbols
    // Simbolos
    json << "  \"symbols\": [\n";
    for (size_t i = 0; i < output.symbols.size(); i++) {
        const auto& sym = output.symbols[i];
        json << "    {\n";
        json << "      \"name\": \"" << escape_json(sym.name) << "\",\n";
        json << "      \"kind\": \"" << escape_json(sym.kind) << "\",\n";
        json << "      \"type_name\": \"" << escape_json(sym.type_name) << "\",\n";
        json << "      \"line\": " << sym.location.line << ",\n";
        json << "      \"col\": " << sym.location.col << ",\n";
        json << "      \"file\": \"" << escape_json(sym.location.file) << "\"\n";
        json << "    }";
        if (i < output.symbols.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n";

    // Errors
    // Erros
    json << "  \"errors\": [\n";
    for (size_t i = 0; i < output.errors.size(); i++) {
        const auto& err = output.errors[i];
        json << "    {\n";
        json << "      \"message\": \"" << escape_json(err.message) << "\",\n";
        json << "      \"line\": " << err.location.line << ",\n";
        json << "      \"col\": " << err.location.col << ",\n";
        json << "      \"file\": \"" << escape_json(err.location.file) << "\",\n";
        json << "      \"severity\": " << err.severity << "\n";
        json << "    }";
        if (i < output.errors.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ]\n";

    json << "}\n";
    return json.str();
}

} // namespace brick
  // namespace brick

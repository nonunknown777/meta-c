#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace brick {

static const std::unordered_map<std::string, TokenType> keywords = {
    {"package", TokenType::PACKAGE},
    {"using",   TokenType::USING},
    {"private", TokenType::PRIVATE},
    {"public",  TokenType::PUBLIC},
    {"struct",  TokenType::STRUCT},
    {"extends", TokenType::EXTENDS},
    {"interface", TokenType::INTERFACE},
    {"fn",      TokenType::FN},
    {"return",  TokenType::RETURN},
    {"if",      TokenType::IF},
    {"else",    TokenType::ELSE},
    {"while",   TokenType::WHILE},
    {"for",     TokenType::FOR},
    {"block",   TokenType::BLOCK},
    {"reset",   TokenType::RESET},
    {"true",    TokenType::TRUE},
    {"false",   TokenType::FALSE},
    {"null",    TokenType::NULL_},
    {"error",   TokenType::ERROR},
    {"int",     TokenType::INT},
    {"float",   TokenType::FLOAT},
    {"bool",    TokenType::BOOL},
    {"char",    TokenType::CHAR},
    {"String",  TokenType::STRING},
    {"void",    TokenType::VOID},
    {"u8",      TokenType::U8},
    {"u16",     TokenType::U16},
    {"u32",     TokenType::U32},
    {"u64",     TokenType::U64},
    {"i8",      TokenType::I8},
    {"i16",     TokenType::I16},
    {"i32",     TokenType::I32},
    {"i64",     TokenType::I64},
    {"f32",     TokenType::F32},
    {"f64",     TokenType::F64},
    {"usize",   TokenType::USIZE},
    {"isize",   TokenType::ISIZE},
    {"byte",    TokenType::BYTE},
};

class Lexer {
public:
    Lexer(const std::string& source, const std::string& filename)
        : source(source), filename(filename), pos(0), line(1), col(1) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < source.size()) {
            skip_whitespace_and_comments();
            if (pos >= source.size()) break;

            Token t = next_token();
            tokens.push_back(t);
        }
        tokens.emplace_back(TokenType::EOF_, "", location());
        return tokens;
    }

private:
    const std::string& source;
    std::string filename;
    size_t pos;
    int line;
    int col;

    SourceLocation location() const {
        return {line, col, filename};
    }

    char peek() const {
        if (pos >= source.size()) return '\0';
        return source[pos];
    }

    char advance() {
        char c = source[pos++];
        if (c == '\n') { line++; col = 1; }
        else { col++; }
        return c;
    }

    void skip_whitespace_and_comments() {
        while (pos < source.size()) {
            if (peek() == '/' && pos + 1 < source.size() && source[pos + 1] == '/') {
                while (pos < source.size() && source[pos] != '\n') advance();
            } else if (std::isspace(peek())) {
                advance();
            } else {
                break;
            }
        }
    }

    Token next_token() {
        SourceLocation loc = location();
        char c = advance();

        // Single-char tokens
        // Tokens de caractere unico
        switch (c) {
            case '{': return {TokenType::LBRACE, "{", loc};
            case '}': return {TokenType::RBRACE, "}", loc};
            case '(': return {TokenType::LPAREN, "(", loc};
            case ')': return {TokenType::RPAREN, ")", loc};
            case '[': return {TokenType::LBRACKET, "[", loc};
            case ']': return {TokenType::RBRACKET, "]", loc};
            case ';': return {TokenType::SEMICOLON, ";", loc};
            case ',': return {TokenType::COMMA, ",", loc};
            case '@': return {TokenType::AT, "@", loc};
            case '.': return {TokenType::DOT, ".", loc};
            case '~': return {TokenType::BIT_NOT, "~", loc};
        }

        // Operators with possible multi-char
        // Operadores com possivel multi-caractere
        if (c == '+') {
            if (peek() == '=') { advance(); return {TokenType::PLUS_ASSIGN, "+=", loc}; }
            if (peek() == '+') { advance(); return {TokenType::PLUS, "++", loc}; }
            return {TokenType::PLUS, "+", loc};
        }
        if (c == '-') {
            if (peek() == '>') { advance(); return {TokenType::ARROW, "->", loc}; }
            if (peek() == '=') { advance(); return {TokenType::MINUS_ASSIGN, "-=", loc}; }
            if (peek() == '-') { advance(); return {TokenType::MINUS, "--", loc}; }
            return {TokenType::MINUS, "-", loc};
        }
        if (c == '*') {
            if (peek() == '=') { advance(); return {TokenType::STAR_ASSIGN, "*=", loc}; }
            return {TokenType::STAR, "*", loc};
        }
        if (c == '/') {
            if (peek() == '=') { advance(); return {TokenType::SLASH_ASSIGN, "/=", loc}; }
            return {TokenType::SLASH, "/", loc};
        }

        if (c == '=') {
            if (peek() == '=') { advance(); return {TokenType::EQ, "==", loc}; }
            return {TokenType::ASSIGN, "=", loc};
        }
        if (c == '!') {
            if (peek() == '=') { advance(); return {TokenType::NEQ, "!=", loc}; }
            return {TokenType::NOT, "!", loc};
        }
        if (c == '<') {
            if (peek() == '=') { advance(); return {TokenType::LEQ, "<=", loc}; }
            if (peek() == '<') { advance(); return {TokenType::LSHIFT, "<<", loc}; }
            return {TokenType::LT, "<", loc};
        }
        if (c == '>') {
            if (peek() == '=') { advance(); return {TokenType::GEQ, ">=", loc}; }
            if (peek() == '>') { advance(); return {TokenType::RSHIFT, ">>", loc}; }
            return {TokenType::GT, ">", loc};
        }
        if (c == '&') {
            if (peek() == '&') { advance(); return {TokenType::AND, "&&", loc}; }
            return {TokenType::BIT_AND, "&", loc};
        }
        if (c == '^') return {TokenType::BIT_XOR, "^", loc};
        if (c == '|') {
            if (peek() == '|') { advance(); return {TokenType::OR, "||", loc}; }
            return {TokenType::BIT_OR, "|", loc};
        }

        // String literal
        // Literal de string
        if (c == '"') return string_literal(loc);
        // Char literal
        // Literal de char
        if (c == '\'') return char_literal(loc);

        // Number literal
        // Literal numerico
        if (std::isdigit(c)) {
            Token t = number_literal(c, loc);
            t.literal_type = parse_literal_suffix();
            return t;
        }

        // Identifier or keyword
        // Identificador ou palavra-chave
        if (std::isalpha(c) || c == '_') return identifier_or_keyword(c, loc);

        // Unexpected character
        // Caractere inesperado
        std::string msg = "unexpected character '" + std::string(1, c) + "'";
        throw std::runtime_error(msg);
    }

    Token string_literal(SourceLocation loc) {
        std::string value;
        while (pos < source.size() && peek() != '"') {
            if (peek() == '\\') {
                advance();
                switch (advance()) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case '\\': value += '\\'; break;
                    case '"': value += '"'; break;
                    default: throw std::runtime_error("invalid escape sequence");
                }
            } else {
                value += advance();
            }
        }
        if (pos >= source.size()) throw std::runtime_error("unterminated string literal");
        advance(); // closing "
                   // fechamento "
        return {TokenType::STRING_LITERAL, value, loc};
    }

    Token char_literal(SourceLocation loc) {
        char c;
        if (peek() == '\\') {
            advance();
            switch (advance()) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case '\\': c = '\\'; break;
                case '\'': c = '\''; break;
                default: throw std::runtime_error("invalid escape sequence");
            }
        } else {
            c = advance();
        }
        if (advance() != '\'') throw std::runtime_error("unterminated char literal");
        return {TokenType::CHAR_LITERAL, std::string(1, c), loc};
    }

    Token number_literal(char first, SourceLocation loc) {
        std::string value(1, first);
        bool is_float = false;
        while (pos < source.size() && (std::isdigit(peek()) || peek() == '.')) {
            if (peek() == '.') {
                if (is_float) break;
                is_float = true;
            }
            value += advance();
        }
        return {is_float ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, value, loc};
    }

    Token identifier_or_keyword(char first, SourceLocation loc) {
        std::string value(1, first);
        while (pos < source.size() && (std::isalnum(peek()) || peek() == '_')) {
            value += advance();
        }
        auto it = keywords.find(value);
        if (it != keywords.end()) {
            return {it->second, value, loc};
        }
        return {TokenType::IDENTIFIER, value, loc};
    }

    bool match_suffix(const std::string& suffix) {
        if (pos + suffix.size() > source.size()) return false;
        for (size_t i = 0; i < suffix.size(); i++) {
            if (source[pos + i] != suffix[i]) return false;
        }
        size_t next = pos + suffix.size();
        if (next < source.size() && (std::isalnum(source[next]) || source[next] == '_')) {
            return false;
        }
        return true;
    }

    std::string parse_literal_suffix() {
        if (pos >= source.size()) return "";
        char c = peek();
        struct { const char* suffix; const char* type; } candidates[] = {
            {"u64", "u64"}, {"u32", "u32"}, {"u16", "u16"}, {"u8", "u8"},
            {"usz", "usize"},
            {"i64", "i64"}, {"i32", "i32"}, {"i16", "i16"}, {"i8", "i8"},
            {"isz", "isize"},
            {"f64", "f64"}, {"f32", "f32"},
        };
        for (auto& cand : candidates) {
            if (cand.suffix[0] == c && match_suffix(cand.suffix)) {
                for (size_t i = 0; cand.suffix[i]; i++) advance();
                return cand.type;
            }
        }
        return "";
    }
};

std::vector<Token> tokenize(const std::string& source, const std::string& filename) {
    Lexer lexer(source, filename);
    return lexer.tokenize();
}

} // namespace brick
  // namespace brick

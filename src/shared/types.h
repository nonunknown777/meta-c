#ifndef BRICK_TYPES_H
#define BRICK_TYPES_H

#include <string>
#include <cstdint>

namespace brick {

enum class TokenType {
    // Keywords
    PACKAGE, USING, PRIVATE, PUBLIC,
    STRUCT, EXTENDS, INTERFACE, FN, RETURN,
    IF, ELSE, WHILE, FOR,
    BLOCK, RESET,
    TRUE, FALSE, NULL_, ERROR,
    INT, FLOAT, BOOL, CHAR, STRING, VOID,
    U8, U16, U32, U64,
    I8, I16, I32, I64,
    F32, F64,
    USIZE, ISIZE,
    BYTE,
    EXTERN, INCLUDE, LINK,

    // Macro keywords
    MACRO, BUILD, EMIT,

    // Literals
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, CHAR_LITERAL,

    // Identifier
    IDENTIFIER,

    // Operators
    PLUS, MINUS, STAR, SLASH, ASSIGN,
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN,
    EQ, NEQ, LT, GT, LEQ, GEQ,
    AND, OR, NOT,
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, LSHIFT, RSHIFT,
    DOT, ARROW, AT, PIPE,

    // Delimiters
    LBRACE, RBRACE, LPAREN, RPAREN,
    LBRACKET, RBRACKET, SEMICOLON, COMMA,

    // Macro symbols
    DOLLAR, ELLIPSIS,

    // Special
    EOF_
};

struct SourceLocation {
    int line;
    int col;
    std::string file;
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;
    std::string literal_type;

    Token() : type(TokenType::EOF_), location{0, 0, ""} {}
    Token(TokenType t, std::string l, SourceLocation loc)
        : type(t), lexeme(std::move(l)), location(loc) {}
};

} // namespace brick

#endif

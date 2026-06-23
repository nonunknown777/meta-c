#include "../src/lexer/lexer.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace brick;

static int passed = 0;
static int failed = 0;

#define TEST(name) void name()
#define RUN(name) do { try { name(); std::cout << "[PASS] " << #name << "\n"; passed++; } catch (const std::exception& e) { std::cout << "[FAIL] " << #name << ": " << e.what() << "\n"; failed++; } } while(0)

TEST(test_basic_tokens) {
    auto tokens = tokenize("int x = 5");
    assert(tokens.size() >= 4);
    assert(tokens[0].type == TokenType::INT);
    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[2].type == TokenType::ASSIGN);
    assert(tokens[3].type == TokenType::INT_LITERAL);
}

TEST(test_string_literal) {
    auto tokens = tokenize("\"hello\"");
    assert(tokens[0].type == TokenType::STRING_LITERAL);
    assert(tokens[0].lexeme == "hello");
}

TEST(test_string_escapes) {
    auto tokens = tokenize("\"a\\nb\\tc\\\\d\\\"\"");
    assert(tokens[0].type == TokenType::STRING_LITERAL);
    assert(tokens[0].lexeme == "a\nb\tc\\d\"");
}

TEST(test_char_literal) {
    auto tokens = tokenize("'x'");
    assert(tokens[0].type == TokenType::CHAR_LITERAL);
    assert(tokens[0].lexeme == "x");
}

TEST(test_char_escape) {
    auto tokens = tokenize("'\\n'");
    assert(tokens[0].type == TokenType::CHAR_LITERAL);
    assert(tokens[0].lexeme == "\n");
}

TEST(test_char_escape_quote) {
    auto tokens = tokenize("'\\''");
    assert(tokens[0].type == TokenType::CHAR_LITERAL);
    assert(tokens[0].lexeme == "'");
}

TEST(test_comments) {
    auto tokens = tokenize("int x // comment\n= 5");
    assert(tokens.size() >= 4);
    assert(tokens[0].type == TokenType::INT);
    assert(tokens[2].type == TokenType::ASSIGN);
}

TEST(test_package_using_dot) {
    auto tokens = tokenize("package SPRITES\nusing SPRITES.EFFECTS");
    assert(tokens[0].type == TokenType::PACKAGE);
    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[2].type == TokenType::USING);
    assert(tokens[3].type == TokenType::IDENTIFIER);
    assert(tokens[4].type == TokenType::DOT);
    assert(tokens[5].type == TokenType::IDENTIFIER);
}

TEST(test_float_literal) {
    auto tokens = tokenize("3.14");
    assert(tokens[0].type == TokenType::FLOAT_LITERAL);
    assert(tokens[0].lexeme == "3.14");
}

TEST(test_int_literal) {
    auto tokens = tokenize("42");
    assert(tokens[0].type == TokenType::INT_LITERAL);
    assert(tokens[0].lexeme == "42");
}

TEST(test_operators_eq_neq) {
    auto tokens = tokenize("== !=");
    assert(tokens[0].type == TokenType::EQ);
    assert(tokens[1].type == TokenType::NEQ);
}

TEST(test_operators_comparison) {
    auto tokens = tokenize("< > <= >=");
    assert(tokens[0].type == TokenType::LT);
    assert(tokens[1].type == TokenType::GT);
    assert(tokens[2].type == TokenType::LEQ);
    assert(tokens[3].type == TokenType::GEQ);
}

TEST(test_operators_logical) {
    auto tokens = tokenize("&& || !");
    assert(tokens[0].type == TokenType::AND);
    assert(tokens[1].type == TokenType::OR);
    assert(tokens[2].type == TokenType::NOT);
}

TEST(test_operators_bitwise) {
    auto tokens = tokenize("& | ^ ~ << >>");
    assert(tokens[0].type == TokenType::BIT_AND);
    assert(tokens[1].type == TokenType::BIT_OR);
    assert(tokens[2].type == TokenType::BIT_XOR);
    assert(tokens[3].type == TokenType::BIT_NOT);
    assert(tokens[4].type == TokenType::LSHIFT);
    assert(tokens[5].type == TokenType::RSHIFT);
}

TEST(test_arrow) {
    auto tokens = tokenize("->");
    assert(tokens[0].type == TokenType::ARROW);
    assert(tokens[0].lexeme == "->");
}

TEST(test_at_pipe) {
    auto tokens = tokenize("@ |");
    assert(tokens[0].type == TokenType::AT);
    assert(tokens[1].type == TokenType::BIT_OR);
}

TEST(test_delimiters) {
    auto tokens = tokenize("{ } ( ) [ ] ; ,");
    assert(tokens[0].type == TokenType::LBRACE);
    assert(tokens[1].type == TokenType::RBRACE);
    assert(tokens[2].type == TokenType::LPAREN);
    assert(tokens[3].type == TokenType::RPAREN);
    assert(tokens[4].type == TokenType::LBRACKET);
    assert(tokens[5].type == TokenType::RBRACKET);
    assert(tokens[6].type == TokenType::SEMICOLON);
    assert(tokens[7].type == TokenType::COMMA);
}

TEST(test_keywords) {
    auto tokens = tokenize("struct extends interface fn return if else while for block private public true false null");
    assert(tokens[0].type == TokenType::STRUCT);
    assert(tokens[1].type == TokenType::EXTENDS);
    assert(tokens[2].type == TokenType::INTERFACE);
    assert(tokens[3].type == TokenType::FN);
    assert(tokens[4].type == TokenType::RETURN);
    assert(tokens[5].type == TokenType::IF);
    assert(tokens[6].type == TokenType::ELSE);
    assert(tokens[7].type == TokenType::WHILE);
    assert(tokens[8].type == TokenType::FOR);
    assert(tokens[9].type == TokenType::BLOCK);
    assert(tokens[10].type == TokenType::PRIVATE);
    assert(tokens[11].type == TokenType::PUBLIC);
    assert(tokens[12].type == TokenType::TRUE);
    assert(tokens[13].type == TokenType::FALSE);
    assert(tokens[14].type == TokenType::NULL_);
}

TEST(test_types) {
    auto tokens = tokenize("int float bool char String void");
    assert(tokens[0].type == TokenType::INT);
    assert(tokens[1].type == TokenType::FLOAT);
    assert(tokens[2].type == TokenType::BOOL);
    assert(tokens[3].type == TokenType::CHAR);
    assert(tokens[4].type == TokenType::STRING);
    assert(tokens[5].type == TokenType::VOID);
}

TEST(test_identifier) {
    auto tokens = tokenize("my_var _private camelCase");
    assert(tokens[0].type == TokenType::IDENTIFIER);
    assert(tokens[0].lexeme == "my_var");
    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[1].lexeme == "_private");
    assert(tokens[2].type == TokenType::IDENTIFIER);
    assert(tokens[2].lexeme == "camelCase");
}

TEST(test_empty_input) {
    auto tokens = tokenize("");
    assert(tokens.size() == 1);
    assert(tokens[0].type == TokenType::EOF_);
}

TEST(test_only_comment) {
    auto tokens = tokenize("// just a comment\n");
    assert(tokens.size() == 1);
    assert(tokens[0].type == TokenType::EOF_);
}

TEST(test_arithmetic) {
    auto tokens = tokenize("+ - * /");
    assert(tokens[0].type == TokenType::PLUS);
    assert(tokens[1].type == TokenType::MINUS);
    assert(tokens[2].type == TokenType::STAR);
    assert(tokens[3].type == TokenType::SLASH);
}

TEST(test_unterminated_string) {
    try {
        tokenize("\"unterminated");
        assert(false && "expected exception");
    } catch (const std::runtime_error&) {
        // expected
        // esperado
    }
}

TEST(test_unterminated_char) {
    try {
        tokenize("'x");
        assert(false && "expected exception");
    } catch (const std::runtime_error&) {
        // expected
        // esperado
    }
}

TEST(test_invalid_escape_string) {
    try {
        tokenize("\"\\q\"");
        assert(false && "expected exception");
    } catch (const std::runtime_error&) {
        // expected
        // esperado
    }
}

TEST(test_invalid_escape_char) {
    try {
        tokenize("'\\q'");
        assert(false && "expected exception");
    } catch (const std::runtime_error&) {
        // expected
        // esperado
    }
}

TEST(test_unexpected_char) {
    try {
        tokenize("#");
        assert(false && "expected exception");
    } catch (const std::runtime_error&) {
        // expected
        // esperado
    }
}

TEST(test_dollar_token) {
    auto tokens = tokenize("$");
    assert(tokens.size() >= 2);
    assert(tokens[0].type == TokenType::DOLLAR);
}

TEST(test_ellipsis_token) {
    auto tokens = tokenize("...");
    assert(tokens.size() >= 2);
    assert(tokens[0].type == TokenType::ELLIPSIS);
}

TEST(test_source_locations) {
    auto tokens = tokenize("int x = 5\nfloat y", "test.brc");
    assert(tokens[0].location.line == 1);
    assert(tokens[0].location.col == 1);
    assert(tokens[0].location.file == "test.brc");
    assert(tokens[4].location.line == 2);
    assert(tokens[4].location.col == 1);
}

int main() {
    RUN(test_basic_tokens);
    RUN(test_string_literal);
    RUN(test_string_escapes);
    RUN(test_char_literal);
    RUN(test_char_escape);
    RUN(test_char_escape_quote);
    RUN(test_comments);
    RUN(test_package_using_dot);
    RUN(test_float_literal);
    RUN(test_int_literal);
    RUN(test_operators_eq_neq);
    RUN(test_operators_comparison);
    RUN(test_operators_logical);
    RUN(test_operators_bitwise);
    RUN(test_arrow);
    RUN(test_at_pipe);
    RUN(test_delimiters);
    RUN(test_keywords);
    RUN(test_types);
    RUN(test_identifier);
    RUN(test_empty_input);
    RUN(test_only_comment);
    RUN(test_arithmetic);
    RUN(test_unterminated_string);
    RUN(test_unterminated_char);
    RUN(test_invalid_escape_string);
    RUN(test_invalid_escape_char);
    RUN(test_unexpected_char);
    RUN(test_source_locations);

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}

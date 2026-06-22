#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include <cassert>
#include <iostream>

using namespace brick;

void test_parse_struct() {
    std::string source = R"(
package TEST

struct Player {
    int hp
    String name

    fn Player(int h, String n) {
        hp = h
        name = n
    }
}
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.ast != nullptr);
    assert(result.errors.empty());
    assert(result.ast->declarations.size() > 0);

    std::cout << "[PASS] test_parse_struct\n";
}

void test_parse_block() {
    std::string source = R"(
package TEST

block global = 256MB
block game = 64MB
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.errors.empty());

    std::cout << "[PASS] test_parse_block\n";
}

void test_parse_if_while() {
    std::string source = R"(
package TEST

fn main() {
    int x = 5 @global

    if (x > 0) {
        x -= 1
    }

    while (x > 0) {
        x -= 1
    }
}
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.errors.empty());

    std::cout << "[PASS] test_parse_if_while\n";
}

void test_fixed_width_types() {
    std::string source = R"(
package TEST

fn test() {
    u8  a = 1
    u16 b = 2
    u32 c = 3
    u64 d = 4
    i8  e = 5
    i16 f = 6
    i32 g = 7
    i64 h = 8
    f32 i = 1.5
    f64 j = 2.5
    usize k = 10
    isize l = 20
    byte m = 255
}
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.errors.empty());

    std::cout << "[PASS] test_fixed_width_types\n";
}

void test_literal_suffixes() {
    std::string source = R"(
package TEST

fn test() {
    int a = 42u8
    int b = 100u32
    int c = 7i16
    int d = 1u64
    float e = 3.14f32
    float f = 2.0f64
    int g = 42usz
    int h = 99isz
}
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.errors.empty());

    std::cout << "[PASS] test_literal_suffixes\n";
}

void test_fixed_width_struct_fields() {
    std::string source = R"(
package TEST

struct Data {
    u8  flag
    i32 count
    f64 precision
    byte raw
}
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.errors.empty());

    std::cout << "[PASS] test_fixed_width_struct_fields\n";
}

int main() {
    test_parse_struct();
    test_parse_block();
    test_parse_if_while();
    test_fixed_width_types();
    test_literal_suffixes();
    test_fixed_width_struct_fields();
    std::cout << "All parser tests passed!\n";
    return 0;
}

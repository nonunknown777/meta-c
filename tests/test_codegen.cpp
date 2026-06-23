#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include "../src/parser/package.h"
#include "../src/codegen/codegen.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <string>

using namespace brick;

static int tests_passed = 0;
static int tests_failed = 0;

void check(bool cond, const std::string& name) {
    if (cond) {
        std::cout << "  [PASS] " << name << "\n";
        tests_passed++;
    } else {
        std::cout << "  [FAIL] " << name << "\n";
        tests_failed++;
    }
}

static bool parse_and_check(const std::string& source, ParseResult& out,
                            const std::string& label) {
    auto tokens = tokenize(source);
    auto parse_result = parse(tokens);
    bool ok = parse_result.errors.empty();
    check(ok, label + " parse");
    if (!ok) {
        for (const auto& e : parse_result.errors)
            std::cerr << "  PARSE ERROR: " << e << "\n";
        return false;
    }
    out = std::move(parse_result);
    return true;
}

static bool codegen_from_ast(ParseResult& parse_result,
                             CodegenResult& out, const std::string& label) {
    auto packages = resolve_packages(parse_result.ast, "test.brc");
    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(parse_result.ast));
    auto cr = generate_c(asts, packages);
    bool ok = cr.success;
    check(ok, label + " codegen");
    if (!ok) {
        for (const auto& e : cr.errors)
            std::cerr << "  CODEGEN ERROR: " << e << "\n";
        return false;
    }
    out = std::move(cr);
    return true;
}

void test_codegen_simple() {
    std::cout << "=== test_codegen_simple ===\n";
    std::string source = R"(
package TEST

block global = 64MB

struct Player {
    int hp

    fn Player(int h) {
        hp = h
    }

    fn get_hp() -> int {
        return hp
    }
}

fn main() {
    Player p = Player(100) @global
    int x = p.get_hp()
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "simple")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "simple")) return;

    std::string c = cr.c_code;
    check(c.find("typedef struct Player") != std::string::npos, "struct definition");
    check(c.find("int32_t hp") != std::string::npos, "field type mapping");
    check(c.find("Player_Player(") != std::string::npos, "constructor call");
    check(c.find("Player_get_hp(") != std::string::npos, "method call");
    check(c.find("pool_alloc(__pool_global, sizeof(Player))") != std::string::npos, "block alloc");
    check(c.find("__brick_init") != std::string::npos, "block create");
    check(c.find("block_create_bytes(67108864)") != std::string::npos, "block create size");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_extends() {
    std::cout << "=== test_codegen_extends ===\n";
    std::string source = R"(
package TEST

block main = 1MB

struct Entity {
    int id
}

struct Player extends Entity {
    int hp

    fn Player(int i, int h) {
        id = i
        hp = h
    }

    fn get_id() -> int {
        return id
    }
}

fn main() {
    Player p = Player(1, 100) @main
    int i = p.get_id()
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "extends")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "extends")) return;

    std::string c = cr.c_code;
    check(c.find("Entity base;") != std::string::npos, "extends base field");
    check(c.find("int32_t hp") != std::string::npos, "field in derived");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_control_flow() {
    std::cout << "=== test_codegen_control_flow ===\n";
    std::string source = R"(
package TEST

block global = 1MB

fn test_if(int x) -> int {
    if (x > 0) {
        return 1
    } else {
        return 0
    }
}

fn test_while(int n) -> int {
    int i = 0
    while (i < n) {
        i = i + 1
    }
    return i
}

fn test_for() -> int {
    int sum = 0
    for (int i = 0; i < 10; i = i + 1) {
        sum = sum + i
    }
    return sum
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "control_flow")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "control_flow")) return;

    std::string c = cr.c_code;
    check(c.find("if (") != std::string::npos, "if statement");
    check(c.find("while (") != std::string::npos, "while statement");
    check(c.find("for (") != std::string::npos, "for statement");
    check(c.find("return ") != std::string::npos, "return statement");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_expressions() {
    std::cout << "=== test_codegen_expressions ===\n";
    std::string source = R"(
package TEST

block global = 1MB

fn test_expr() {
    int a = 10
    int b = 20
    bool c = true
    bool d = false
    int sum = a + b
    int diff = a - b
    int prod = a * b
    int quot = a / b
    bool eq = a == b
    bool neq = a != b
    bool lt = a < b
    bool gt = a > b
    bool leq = a <= b
    bool geq = a >= b
    bool and = c && d
    bool or = c || d
    bool not = !c
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "expressions")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "expressions")) return;

    std::string c = cr.c_code;
    check(c.find("10") != std::string::npos, "int literal");
    check(c.find("&&") != std::string::npos, "and operator");
    check(c.find("||") != std::string::npos, "or operator");
    check(c.find("!") != std::string::npos, "not operator");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_string() {
    std::cout << "=== test_codegen_string ===\n";
    std::string source = R"(
package TEST

block global = 1MB

fn test_string() {
    String s = "hello"
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "string")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "string")) return;

    std::string c = cr.c_code;
    check(c.find("BrickString") != std::string::npos, "BrickString type");
    check(c.find("hello") != std::string::npos, "string literal");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_null() {
    std::cout << "=== test_codegen_null ===\n";
    std::string source = R"(
package TEST

block global = 1MB

struct Node {
    int value
    int next
}

fn test_null() {
    int x = null
    Node n = null @global
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "null")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "null")) return;

    std::string c = cr.c_code;
    check(c.find("NULL") != std::string::npos, "null literal");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_block_scope() {
    std::cout << "=== test_codegen_block_scope ===\n";
    std::string source = R"(
package TEST

block global = 64MB
block game = 16MB

fn main() {
    int x = 10 @global
    block game {
        int y = 20 @game
    }
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "block_scope")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "block_scope")) return;

    std::string c = cr.c_code;
    check(c.find("_current_block") != std::string::npos, "block scope push/pop");
    check(c.find("game") != std::string::npos, "block name reference");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_type_errors() {
    std::cout << "=== test_codegen_type_errors ===\n";
    std::string source = R"(
package TEST

fn bad_return() -> int {
    return "string"
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "type_error")) return;
    auto packages = resolve_packages(pr.ast, "test.brc");
    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(pr.ast));
    auto cr = generate_c(asts, packages);
    check(!cr.success, "should fail type check");
    check(!cr.errors.empty(), "should have errors");

    if (!cr.errors.empty()) {
        std::cout << "  Expected errors:\n";
        for (const auto& e : cr.errors)
            std::cout << "    " << e << "\n";
    }
}

void test_codegen_undefined_symbol() {
    std::cout << "=== test_codegen_undefined_symbol ===\n";
    std::string source = R"(
package TEST

fn test() {
    int undefined_var = 42
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "undef_sym")) return;
    auto packages = resolve_packages(pr.ast, "test.brc");
    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(pr.ast));
    auto cr = generate_c(asts, packages);
    check(cr.success, "should pass type check (var is declared)");
}

void test_codegen_compile_c() {
    std::cout << "=== test_codegen_compile_c ===\n";
    std::string source = R"(
package TEST

block global = 64MB

struct Player {
    int hp

    fn Player(int h) {
        hp = h
    }

    fn get_hp() -> int {
        return hp
    }

    fn take_damage(int dmg) {
        hp = hp - dmg
    }
}

fn main() {
    Player p = Player(100) @global
    int x = p.get_hp()
    p.take_damage(10)
    int y = p.get_hp()
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "compile_c")) return;
    auto packages = resolve_packages(pr.ast, "test.brc");
    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(pr.ast));
    auto cr = generate_c(asts, packages);
    check(cr.success, "codegen success");
    if (!cr.success) return;

    std::string c_code = R"(
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef struct BlockCtx {
    uint8_t* data;
    size_t capacity;
    size_t used;
    size_t peak_used;
    size_t allocation_count;
} BlockCtx;

BlockCtx* block_create_bytes(size_t bytes) {
    BlockCtx* ctx = (BlockCtx*)malloc(sizeof(BlockCtx));
    ctx->data = (uint8_t*)malloc(bytes);
    ctx->capacity = bytes;
    ctx->used = 0;
    ctx->peak_used = 0;
    ctx->allocation_count = 0;
    return ctx;
}
BlockCtx* block_create(size_t megabytes) {
    return block_create_bytes(megabytes * 1024 * 1024);
}

void* block_alloc(BlockCtx* ctx, size_t size) {
    void* ptr = ctx->data + ctx->used;
    ctx->used += size;
    ctx->allocation_count++;
    if (ctx->used > ctx->peak_used) ctx->peak_used = ctx->used;
    return ptr;
}

void block_reset(BlockCtx* ctx) { ctx->used = 0; }
void block_destroy(BlockCtx* ctx) { free(ctx->data); free(ctx); }
void block_set_tls(BlockCtx* ctx) { (void)ctx; }
#define block_register(ctx, name)     ((void)(ctx), (void)(name))
#define block_unregister(ctx)         ((void)(ctx))
#define block_find(name)              ((void)(name), (BlockCtx*)NULL)
#define block_snapshot(out, max)      ((void)(out), (void)(max), (size_t)0)
#define block_shm_export()            ((void)0)
// Pool allocator stubs for compilation test
// Stubs do pool allocator para o teste de compilacao
typedef struct { void* data; } PoolBlockHeader;
typedef struct { int dummy; } PoolAllocator;
PoolAllocator* pool_create(void) { return (PoolAllocator*)calloc(1, 64); }
int pool_add_slot(PoolAllocator* p, size_t bs, size_t ct) { (void)p; (void)bs; (void)ct; return 0; }
void* pool_alloc(PoolAllocator* p, size_t s) { (void)p; return malloc(s); }
void pool_free(PoolAllocator* p, void* ptr) { (void)p; free(ptr); }
void pool_destroy(PoolAllocator* p) { free(p); }
)";
    c_code += cr.c_code;

    // The generated code already defines all the functions and structs.
    // O codigo gerado ja define todas as funcoes e structs.
    // Compile it directly (the test provides BlockCtx stubs in c_code prefix).
    // Compila diretamente (o teste fornece stubs de BlockCtx no prefixo c_code).
    // Remove the block_memory.h include since we inline stubs.
    // Remove o include de block_memory.h ja que usamos stubs inline.
    {
        std::string needle = "#include \"block_memory.h\"\n";
        size_t p = c_code.find(needle);
        if (p != std::string::npos)
            c_code.erase(p, needle.size());
    }
    // Remove the pool_allocator.h include since we inline stubs.
    // Remove o include de pool_allocator.h ja que usamos stubs inline.
    {
        std::string needle = "#include \"pool_allocator.h\"\n";
        size_t p = c_code.find(needle);
        if (p != std::string::npos)
            c_code.erase(p, needle.size());
    }

    FILE* f = fopen("/tmp/test_codegen_output.c", "w");
    assert(f);
    fputs(c_code.c_str(), f);
    fclose(f);

    std::string inc_path = std::string(PROJECT_ROOT) + "/runtime";
    std::string cmd = "gcc -O3 -Wall -Werror -Wno-unused-variable -Wno-main "
                      "-I" + inc_path + " -c /tmp/test_codegen_output.c "
                      "-o /tmp/test_codegen_output.o 2>&1";
    int ret = system(cmd.c_str());
    check(ret == 0, "compilation with gcc -O3 -Wall -Werror");

    if (ret != 0) {
        system(cmd.c_str());
        std::cout << "  Failed C code:\n" << c_code << "\n";
    }
}

void test_codegen_print() {
    std::cout << "=== test_codegen_print ===\n";
    std::string source = R"(
package TEST

using IO

fn main() {
    print("hello")
    print(42)
    print(3.14)
    print(true)
    print('a')
    print()
    print("x = {0}, y = {1}", 10, "test")
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "print")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "print")) return;

    std::string c = cr.c_code;
    check(c.find("io_print_string") != std::string::npos, "io_print_string");
    check(c.find("io_print_i32") != std::string::npos, "io_print_i32");
    check(c.find("io_print_f32") != std::string::npos, "io_print_f32");
    check(c.find("io_print_bool") != std::string::npos, "io_print_bool");
    check(c.find("io_print_char") != std::string::npos, "io_print_char");
    check(c.find("io_print_newline") != std::string::npos, "io_print_newline");
    check(c.find("io_printf") != std::string::npos, "io_printf format");
    check(c.find("#include \"io.h\"") != std::string::npos, "io.h include");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_print_without_using() {
    std::cout << "=== test_codegen_print_without_using ===\n";
    std::string source = R"(
package TEST

fn main() {
    print("hello")
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "print_no_using")) return;
    auto packages = resolve_packages(pr.ast, "test.brc");
    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(pr.ast));
    auto cr = generate_c(asts, packages);
    check(!cr.success, "should fail without using IO");
    if (!cr.errors.empty()) {
        std::cout << "  Expected error: " << cr.errors[0] << "\n";
    }
}

void test_codegen_fixed_width_types() {
    std::cout << "=== test_codegen_fixed_width_types ===\n";
    std::string source = R"(
package TEST

block global = 1MB

fn test_types() {
    i8 a = 127
    i16 b = 32767
    i32 c = 2147483647
    i64 d = 9223372036854775807
    u8 e = 255
    u16 f = 65535
    u32 g = 4294967295
    f32 h = 3.14f32
    f64 i = 3.14
    usize j = 0
    isize k = 0
    byte l = 0
    short m = 0
    long n = 0
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "fixed_width")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "fixed_width")) return;

    std::string c = cr.c_code;
    check(c.find("int8_t") != std::string::npos, "i8 -> int8_t");
    check(c.find("int16_t") != std::string::npos, "i16 -> int16_t");
    check(c.find("int32_t") != std::string::npos, "i32 -> int32_t");
    check(c.find("int64_t") != std::string::npos, "i64 -> int64_t");
    check(c.find("uint8_t") != std::string::npos, "u8 -> uint8_t");
    check(c.find("uint16_t") != std::string::npos, "u16 -> uint16_t");
    check(c.find("uint32_t") != std::string::npos, "u32 -> uint32_t");
    check(c.find("float") != std::string::npos, "f32 -> float");
    check(c.find("double") != std::string::npos, "f64 -> double");
    check(c.find("size_t") != std::string::npos, "usize -> size_t");
    check(c.find("ptrdiff_t") != std::string::npos, "isize -> ptrdiff_t");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_literal_suffix() {
    std::cout << "=== test_codegen_literal_suffix ===\n";
    std::string source = R"(
package TEST

block global = 1MB

fn test_suffix() {
    u8 a = 42u8
    i16 b = 1000i16
    f32 c = 2.5f32
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "literal_suffix")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "literal_suffix")) return;

    std::string c = cr.c_code;
    check(c.find("(uint8_t)42") != std::string::npos, "u8 suffix cast");
    check(c.find("(int16_t)1000") != std::string::npos, "i16 suffix cast");
    check(c.find("(float)2.5") != std::string::npos, "f32 suffix cast");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_codegen_literal_overflow() {
    std::cout << "=== test_codegen_literal_overflow ===\n";
    std::string source = R"(
package TEST

fn test_overflow() {
    u8 a = 300
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "literal_overflow")) return;
    auto packages = resolve_packages(pr.ast, "test.brc");
    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(pr.ast));
    auto cr = generate_c(asts, packages);
    check(!cr.success, "should fail on overflow");
    if (!cr.errors.empty()) {
        std::cout << "  Expected error: " << cr.errors[0] << "\n";
    }
}

void test_codegen_type_promotion() {
    std::cout << "=== test_codegen_type_promotion ===\n";
    std::string source = R"(
package TEST

block global = 1MB

fn test_promotion() -> i64 {
    i8 a = 10
    u16 b = 20
    i32 c = a + b   // i8 + u16 -> i32
    return c
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "type_promotion")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "type_promotion")) return;

    std::string c = cr.c_code;
    check(c.find("int32_t") != std::string::npos, "i8+u16 -> i32");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_optimization_inline_hints() {
    std::cout << "=== test_optimization_inline_hints ===\n";
    std::string source = R"(
package TEST

block global = 64MB

struct Foo {
    int x

    fn get_x() -> int {
        return x
    }
}

fn add(int a, int b) -> int {
    return a + b
}

fn main() {
    int r = add(1, 2)
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "inline_hints")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "inline_hints")) return;

    std::string c = cr.c_code;
    check(c.find("__attribute__((always_inline))") != std::string::npos,
          "always_inline attribute present");
    check(c.find("static inline") != std::string::npos,
          "static inline present");
    check(c.find("static inline") < c.find("main(") ||
          c.find("__attribute__((always_inline))") < c.find("main("),
          "inline hint before non-main functions");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_optimization_simd_alignment() {
    std::cout << "=== test_optimization_simd_alignment ===\n";
    std::string source = R"(
package TEST

block global = 64MB

struct Particles {
    f32[4] positions
    f64[2] velocities
    f32   temperature
    f64   mass
    int   count
}

fn main() {}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "simd_alignment")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "simd_alignment")) return;

    std::string c = cr.c_code;
    check(c.find("__attribute__((aligned(16)))") != std::string::npos ||
          c.find("__attribute__((aligned(32)))") != std::string::npos,
          "SIMD alignment attribute on float arrays");
    check(c.find("aligned") != std::string::npos, "at least one aligned attribute");
    std::cout << "  Generated C:\n" << c << "\n";
}

void test_optimization_constant_folding() {
    std::cout << "=== test_optimization_constant_folding ===\n";
    std::string source = R"(
package TEST

block global = 64MB

fn main() {
    int x = 10 + 20
    int y = 100 - 30
    int z = 5 * 7
}
)";

    ParseResult pr;
    if (!parse_and_check(source, pr, "constant_folding")) return;
    CodegenResult cr;
    if (!codegen_from_ast(pr, cr, "constant_folding")) return;

    std::string c = cr.c_code;
    // The folded constants should be integers, not expressions
    check(c.find("= (10 + 20)") == std::string::npos, "10+20 folded (no expression)");
    check(c.find("= 30") != std::string::npos, "10+20 -> 30");
    check(c.find("= 70") != std::string::npos, "100-30 -> 70");
    check(c.find("= 35") != std::string::npos, "5*7 -> 35");
    std::cout << "  Generated C:\n" << c << "\n";
}

int main() {
    test_codegen_simple();
    test_codegen_extends();
    test_codegen_control_flow();
    test_codegen_expressions();
    test_codegen_string();
    test_codegen_null();
    test_codegen_block_scope();
    test_codegen_type_errors();
    test_codegen_undefined_symbol();
    test_codegen_print();
    test_codegen_print_without_using();
    test_codegen_compile_c();
    test_codegen_fixed_width_types();
    test_codegen_literal_suffix();
    test_codegen_literal_overflow();
    test_codegen_type_promotion();

    // Optimization tests
    test_optimization_inline_hints();
    test_optimization_simd_alignment();
    test_optimization_constant_folding();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <vector>
#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include "../src/parser/macro_expander.h"
#include "../src/parser/build_eval.h"

using namespace brick;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    std::cout << "  TEST: " << name << "... "; \
    try {

#define END_TEST \
        std::cout << "PASS\n"; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAIL (" << e.what() << ")\n"; \
    } catch (...) { \
        std::cout << "FAIL (unknown)\n"; \
    } \
} while(0)





// ─── Tests ───

void test_lex_macro_keywords() {
    auto tokens = tokenize("macro hello(a, b) { x = a }\nbuild { }\nemit { x = 1 }", "test");
    bool has_macro = false, has_build = false, has_emit = false;
    for (auto& t : tokens) {
        if (t.type == TokenType::MACRO) has_macro = true;
        if (t.type == TokenType::BUILD) has_build = true;
        if (t.type == TokenType::EMIT) has_emit = true;
    }
    assert(has_macro);
    assert(has_build);
    assert(has_emit);
}

void test_lex_dollar_ellipsis() {
    auto tokens = tokenize("$name $(expr) values...", "test");
    bool has_dollar = false, has_ellipsis = false;
    for (auto& t : tokens) {
        if (t.type == TokenType::DOLLAR) has_dollar = true;
        if (t.type == TokenType::ELLIPSIS) has_ellipsis = true;
    }
    assert(has_dollar);
    assert(has_ellipsis);
}

void test_parse_macro_simple() {
    auto tokens = tokenize(
        "macro swap(a, b) {\n"
        "    tmp = $a\n"
        "    $a = $b\n"
        "    $b = tmp\n"
        "}\n"
        "fn main() {\n"
        "    x = 1\n"
        "    y = 2\n"
        "    swap(x, y)\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
    assert(result.errors.empty());
}

void test_parse_macro_varargs() {
    auto tokens = tokenize(
        "macro my_print(fmt, values...) {\n"
        "    // varargs test\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
    assert(result.errors.empty());
}

void test_parse_build_block() {
    auto tokens = tokenize(
        "build {\n"
        "    x = 42\n"
        "    print(\"hello\")\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
    assert(result.errors.empty());
}

void test_parse_emit_stmt() {
    auto tokens = tokenize(
        "macro gen() {\n"
        "    emit { x = 1 }\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
    assert(result.errors.empty());
}

void test_parse_emit_macro_call() {
    auto tokens = tokenize(
        "macro gen() {\n"
        "    emit other_macro()\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
    assert(result.errors.empty());
}

void test_collect_macros() {
    auto tokens = tokenize(
        "macro swap(a, b) {\n"
        "    tmp = $a\n"
        "    $a = $b\n"
        "    $b = tmp\n"
        "}\n"
        "fn main() {}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);
    assert(table.size() == 1);
    assert(table.count("swap") == 1);
}

void test_collect_multiple_macros() {
    auto tokens = tokenize(
        "macro a() { x = 1 }\n"
        "macro b() { x = 2 }\n"
        "macro c() { x = 3 }\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);
    assert(table.size() == 3);
}

void test_expand_simple_macro() {
    auto tokens = tokenize(
        "macro swap(a, b) {\n"
        "    tmp = $a\n"
        "    $a = $b\n"
        "    $b = tmp\n"
        "}\n"
        "fn main() {\n"
        "    x = 1\n"
        "    y = 2\n"
        "    swap(x, y)\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);
    assert(table.size() == 1);

    auto expand = expand_macros(asts, table);
    assert(expand.success);
}

void test_expand_macro_undefined_error() {
    auto tokens = tokenize(
        "fn main() {\n"
        "    undefined_macro(1, 2)\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    // Undefined macro calls are treated as regular function calls (no error)
    // Chamadas de macro nao definidas sao tratadas como chamadas de funcao normais
    assert(expand.success);
}

void test_build_eval_basic() {
    auto tokens = tokenize(
        "fn main() {\n"
        "    x = 1\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    auto eval = eval_build_blocks(asts, table);
    assert(eval.success);
}

void test_build_eval_with_literals() {
    auto tokens = tokenize(
        "build {\n"
        "    x = 42\n"
        "    y = x + 10\n"
        "    emit { z = y }\n"
        "}\n"
        "fn main() {}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    auto eval = eval_build_blocks(asts, table);
    assert(eval.success);
}

void test_macro_with_emit() {
    auto tokens = tokenize(
        "macro gen_val(n) {\n"
        "    emit { x = $n }\n"
        "}\n"
        "fn main() {\n"
        "    gen_val(42)\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);
    assert(table.size() == 1);

    auto expand = expand_macros(asts, table);
    assert(expand.success);
}

void test_empty_build() {
    auto tokens = tokenize("build {}\nfn main() {}\n", "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    auto eval = eval_build_blocks(asts, table);
    assert(eval.success);
}

void test_dollar_in_macro() {
    auto tokens = tokenize(
        "macro id(x) { $x }\n"
        "fn main() {}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
}

void test_full_integration_swap() {
    auto tokens = tokenize(
        "macro swap(a, b) {\n"
        "    tmp = $a\n"
        "    $a = $b\n"
        "    $b = tmp\n"
        "}\n"
        "fn main() {\n"
        "    x = 1\n"
        "    y = 2\n"
        "    swap(x, y)\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
    assert(result.errors.empty());

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    assert(expand.success);
    assert(expand.errors.empty());

    // After expansion, no macro decls or calls should remain
    // Apos expansao, nenhuma declaracao ou chamada de macro deve permanecer
    for (auto& ast : asts) {
        for (auto& d : ast->declarations) {
            assert(d->type != ASTNodeType::MACRO_DECL);
            assert(d->type != ASTNodeType::MACRO_CALL);
        }
    }
}

void test_macro_no_params() {
    auto tokens = tokenize(
        "macro constant() {\n"
        "    emit { x = 42 }\n"
        "}\n"
        "fn main() {\n"
        "    constant()\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    assert(expand.success);
}

void test_macro_wrong_arg_count() {
    auto tokens = tokenize(
        "macro two(a, b) {\n"
        "    x = $a + $b\n"
        "}\n"
        "fn main() {\n"
        "    two(1)\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    // Wrong arg count should be an error
    // Contagem errada de argumentos deve ser um erro
    assert(!expand.success);
}

void test_macro_deep_recursion() {
    auto tokens = tokenize(
        "macro recurse() {\n"
        "    recurse()\n"
        "}\n"
        "fn main() {\n"
        "    recurse()\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    // Deep recursion should be trapped
    // Recursao profunda deve ser capturada
    assert(!expand.success);
}

void test_clone_ast() {
    auto* orig = new IntLiteral(42, {1, 1, "test"});
    auto cloned = clone_ast(orig);
    assert(cloned != nullptr);
    assert(cloned->type == ASTNodeType::INT_LITERAL);
    assert(static_cast<IntLiteral*>(cloned.get())->value == 42);
    delete orig;
}

void test_clone_ast_block() {
    auto tokens = tokenize("fn main() { x = 1; y = 2 }\n", "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    auto cloned = clone_ast(result.ast.get());
    assert(cloned != nullptr);
    assert(cloned->type == ASTNodeType::PROGRAM);
}

void test_gensym_basic() {
    // Gensym: __var → var__1, var__2
    // We test this indirectly through macro expansion with __ variables
    auto tokens = tokenize(
        "macro with_gensym() {\n"
        "    __tmp = 42\n"
        "    print(__tmp)\n"
        "}\n"
        "fn main() {\n"
        "    with_gensym()\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    assert(expand.success);
}

void test_parse_enum_in_macro() {
    // Enum-style pattern: define multiple constants from a single call
    auto tokens = tokenize(
        "macro enum(name, values...) {}\n"
        "fn main() {}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);
    assert(result.errors.empty());
}

void test_macro_call_as_stmt() {
    auto tokens = tokenize(
        "macro say_hello() {\n"
        "    emit { print(\"hello\") }\n"
        "}\n"
        "fn main() {\n"
        "    say_hello()\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    assert(expand.success);
}

void test_macro_nested() {
    auto tokens = tokenize(
        "macro inner(x) { $x = $x + 1 }\n"
        "macro outer(y) {\n"
        "    inner($y)\n"
        "}\n"
        "fn main() {\n"
        "    v = 1\n"
        "    outer(v)\n"
        "}\n",
    "test");
    auto result = parse(tokens);
    assert(result.ast != nullptr);

    std::vector<std::unique_ptr<ProgramNode>> asts;
    asts.push_back(std::move(result.ast));
    MacroTable table;
    collect_macros(asts, table);

    auto expand = expand_macros(asts, table);
    assert(expand.success);
}

int main() {
    std::cout << "=== Macro System Tests ===\n\n";

    std::cout << "--- Lexer ---\n";
    test_lex_macro_keywords();
    test_lex_dollar_ellipsis();

    std::cout << "--- Parser ---\n";
    test_parse_macro_simple();
    test_parse_macro_varargs();
    test_parse_build_block();
    test_parse_emit_stmt();
    test_parse_emit_macro_call();
    test_parse_enum_in_macro();
    test_dollar_in_macro();

    std::cout << "--- Macro Collection ---\n";
    test_collect_macros();
    test_collect_multiple_macros();

    std::cout << "--- Macro Expansion ---\n";
    test_expand_simple_macro();
    test_expand_macro_undefined_error();
    test_macro_wrong_arg_count();
    test_macro_with_emit();
    test_macro_no_params();
    test_macro_deep_recursion();
    test_macro_call_as_stmt();
    test_macro_nested();
    test_full_integration_swap();

    std::cout << "--- Build Eval ---\n";
    test_empty_build();
    test_build_eval_basic();
    test_build_eval_with_literals();

    std::cout << "--- Clone ---\n";
    test_clone_ast();
    test_clone_ast_block();

    std::cout << "--- Gensym ---\n";
    test_gensym_basic();

    std::cout << "\n=== Results: " << tests_passed << "/" << tests_run << " passed ===\n";
    return tests_passed == tests_run ? 0 : 1;
}

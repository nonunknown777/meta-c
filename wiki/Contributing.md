# Contributing to Brick

Thank you for your interest in contributing to Brick! This guide covers everything you need to know to get started.

---

## Project Overview

Brick is a programming language that compiles to C, with a focus on performance, block-based memory management, and hot reload. The project is structured into 11 tasks, each with its own area of responsibility.

| Task | Component | Language | Description |
|:----:|-----------|:--------:|-------------|
| 01 | Lexer | C++20 | Tokenize `.brc` files |
| 02 | Parser | C++20 | Build AST + package resolution |
| 03 | Codegen | C++20 | Type checking + C code generation |
| 04 | Runtime | C | Block memory allocator |
| 05 | Hot Reload | C | dlopen + inotify engine |
| 06 | Visualizer | C++ | ncurses TUI for memory blocks |
| 07 | Builder | Python | SCons build system |
| 08 | VS Code Ext | TS/JSON | Extension with LSP + debug webview |
| 09 | Debugger | Python | GDB pretty-printers + commands |
| 10 | Tester/Optimizer | Various | Test, optimize, document, coordinate |
| 11 | Libraries | C/other | Official libraries (window, input, net, etc.) |

---

## Setting Up Development Environment

### Requirements

```bash
# Essential
sudo pacman -S gcc scons    # Arch
sudo apt install g++ scons  # Ubuntu/Debian

# Optional but recommended
sudo pacman -S ncurses libx11  # For visualizer + window library
```

### Clone and Build

```bash
git clone <repo-url> brick
cd brick

# Build the compiler
scons

# Or debug build
scons profile=debug

# Run tests
scons test
tests/test_integration.sh
```

### Workflow

```bash
# Typical development cycle:
# 1. Make changes
# 2. Build
scons
# 3. Run tests
scons test
# 4. Integration test
tests/test_integration.sh
# 5. Commit
git add -p
git commit
```

---

## Coding Conventions

### General

- **C++20** for the compiler (`src/`)
- **C11** for the runtime (`runtime/`)
- **Python 3** for build scripts and debugger
- **TypeScript** for VS Code extension

### C++ Conventions

```cpp
// Namespace
namespace brick {

// Functions: snake_case
void block_alloc(BlockCtx* ctx);

// Structs/Classes: PascalCase
struct BlockCtx {
    int64_t capacity;
};

// Constants: UPPER_SNAKE_CASE
constexpr int DEFAULT_ALIGNMENT = 8;

// Enums: PascalCase + members UPPER_SNAKE_CASE
enum class TokenType {
    INT_LITERAL,
    FLOAT_LITERAL,
};

} // namespace brick
```

### C Conventions

```c
// Functions: snake_case with module prefix
void block_reset(BlockCtx* ctx);
void io_print_int(int64_t val);

// Structs: PascalCase
typedef struct BlockCtx { ... } BlockCtx;

// always use extern "C" in headers for C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif
// ...
#ifdef __cplusplus
}
#endif
```

### Brick Language Conventions (for examples/tests)

- 4-space indentation
- `snake_case` for functions and variables
- `PascalCase` for structs and packages
- No `this` keyword
- No name shadowing

### File Headers

Every source file should include a brief comment:

```cpp
// Brick — <brief description of what this file does>
```

---

## Testing

### Running Tests

```bash
# Unit tests only
scons test

# Integration tests
tests/test_integration.sh

# Benchmarks
benchmarks/run_benchmarks.sh
```

### Writing Unit Tests

Unit tests are in `tests/`. Each component has its own test file:

```
tests/
├── test_lexer.cpp       # Tests tokenization
├── test_parser.cpp      # Tests AST building
├── test_codegen.cpp     # Tests code generation
├── test_runtime.c       # Tests block memory allocator
├── test_hot_reload.c    # Tests hot reload engine
└── test_integration.sh  # End-to-end tests
```

**Example unit test pattern** (C++ tests use custom test harness, no external framework):

```cpp
// test_lexer.cpp
#include "../src/lexer/lexer.h"
#include <cassert>

namespace brick {

void test_simple_tokens() {
    auto tokens = tokenize("int x = 5");
    assert(tokens.size() == 5);
    assert(tokens[0].type == TokenType::INT);
    assert(tokens[1].type == TokenType::IDENTIFIER);
    assert(tokens[1].lexeme == "x");
    assert(tokens[2].type == TokenType::ASSIGN);
    assert(tokens[3].type == TokenType::INT_LITERAL);
    assert(tokens[3].lexeme == "5");
    printf("PASS: test_simple_tokens\n");
}

} // namespace brick

int main() {
    brick::test_simple_tokens();
    return 0;
}
```

### Writing Integration Tests

Integration tests compile `.brc` → C → binary → run and check output:

```bash
# Add to test_integration.sh:
test_compile_and_expect "my_test" "path/to/test.brc" "Expected output"
```

### Test Coverage Guidelines

- Every new language feature needs a unit test
- Every new runtime API needs a unit test
- Integration tests for end-to-end compilation + execution
- Benchmark tests for performance-sensitive changes

---

## Build System

Brick uses **SCons** (Python). Key files:

```
SConstruct           → Entry point, build profiles, platform detection
src/SConscript       → Compiler library + CLI
runtime/SConscript   → Runtime library
tests/SConscript     → Test binaries
visualizer/SConscript → TUI visualizer
```

### Adding a New Source File

1. Add the source file to the appropriate directory
2. Update the corresponding `SConscript` to include it
3. Run `scons` to test

Example — adding a new file to `src/codegen/`:

```python
# src/SConscript — add to the library
lib_codegen = env.Library('#build/brick_codegen', [
    'codegen/codegen.cpp',
    'codegen/type_checker.cpp',
    'codegen/new_feature.cpp',  # ← add here
])
```

### Adding a New Test

1. Create `tests/test_my_feature.cpp` (or `.c`)
2. Update `tests/SConscript` to build it:

```python
test_bins.append(build_test('test_my_feature', ['test_my_feature.cpp'],
                            ['brick_codegen', 'brick_parser', 'brick_lexer']))
```

3. Run `scons test`

---

## Pull Request Workflow

### 1. Fork and Clone

```bash
git clone <your-fork-url> brick
cd brick
```

### 2. Create a Branch

```bash
git checkout -b fix/description
# or
git checkout -b feat/description
```

### 3. Make Changes

- Follow [coding conventions](#coding-conventions)
- Add tests for new functionality
- Make sure existing tests still pass

### 4. Verify

```bash
# Full test suite
scons test
tests/test_integration.sh

# Benchmarks (if performance-relevant)
benchmarks/run_benchmarks.sh
```

### 5. Commit

```bash
git add -p   # Review each change
git commit -m "component: brief description of changes"
```

**Commit message format:**
```
<component>: <brief description>

Optional longer description of what and why.
```

Examples:
```
lexer: support hex integer literals (0xFF)
parser: fix infinite loop on unterminated block comments
runtime: add thread-safe block alloc option
codegen: emit line directives for constructor bodies
```

### 6. Push and Create PR

```bash
git push origin fix/description
```

Then create a Pull Request on GitHub with:
- A clear description of the change
- Any relevant issue numbers
- Testing notes

### 7. PR Review Checklist

- [ ] Code follows conventions
- [ ] Tests pass
- [ ] New features have tests
- [ ] Performance impact considered (if relevant)
- [ ] Documentation updated (if needed)
- [ ] No new warnings from compiler/sanitizer

---

## Component-Specific Guidelines

### Lexer (Task 01)

- New keywords: add to `keywords` map in `lexer.cpp` and `TokenType` enum in `types.h`
- New operators: add tokenizing logic in `next_token()` switch
- Always track `SourceLocation` (line, col, file)

### Parser (Task 02)

- New AST nodes: add struct in `ast.h`, parsing logic in `parser.cpp`
- Recursive descent: one function per grammar rule
- Error recovery: parse errors should be collected, not thrown
- Package resolution: new package features go in `package.cpp`

### Codegen (Task 03)

- New C output: add generation methods to `Codegen` class
- Type checking: new type rules in `TypeChecker` class
- Type mapping: update `map_type()` in `codegen.cpp`
- Generated C must be valid C11 and compile with `gcc -O3 -Wall`

### Runtime (Task 04)

- New runtime features must be in C (not C++)
- Header must use `extern "C"` guards
- Performance-critical code should be measurable via benchmarks
- Block memory changes must maintain O(1) alloc and reset

### Hot Reload (Task 05)

- Must handle reload failures gracefully (rollback)
- Must not leak file descriptors
- Thread safety is critical (atomic operations for function pointers)
- Block freeze/thaw must work correctly

### Visualizer (Task 06)

- ncurses-based TUI
- Must handle terminal resize
- Attach mode reads from shared memory file
- Standalone mode (no running program) should show demo data

### Debugger (Task 09)

- All Python code must work with GDB's embedded Python
- Pretty-printers must handle invalid data gracefully
- `.gdbinit` must auto-detect the project root

---

## Documentation Conventions

- Documentation is primarily in **English**
- Code comments should be brief and explain **why**, not **what** (the code should be clear enough for "what")
- Each major component should have a `.md` file in `docs/`
- The wiki is the user-facing documentation
- For bilingual docs (Portuguese/English), see existing files for format

---

## Getting Help

If you're stuck:

1. Read the existing code — it's intentionally readable
2. Check `shared-context.md` for the full language specification
3. Look at `docs/` for component documentation
4. Read the relevant task's `STATE.md` for current progress
5. Open an issue on GitHub

---

## Code Review Philosophy

- **Readability matters**: Code is written once but read many times
- **Tests are non-negotiable**: Every feature must be testable
- **Performance by default**: Optimize as you write, not as an afterthought
- **No bloat**: Don't add dependencies unless absolutely necessary
- **Consistency**: Follow existing patterns, not personal preferences
- **Simple over clever**: If you can't explain your code in 30 seconds, it's too complex

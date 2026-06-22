# Getting Started with Brick

> Quick guide for anyone who wants to use or contribute to the language.

## What You Need

- **Linux** (any distro)
- **g++** with C++20 support (GCC >= 11 or Clang >= 14)
- **gcc** to compile generated code
- **SCons** (`pip install scons`)
- **ncurses** for the visualizer (optional)

## Build the Compiler

```bash
git clone https://github.com/nonunknown777/brick.git
cd brick
scons                     # build release / release build
scons profile=debug       # build debug / debug build
```

> The `brick` binary will be at `build/brick`.

## Compile and Run a Brick Program

### Quickest way

```bash
brick run examples/hello.brc
```

> This compiles `.brc` → C → binary and runs it in one step.

### Build to binary

```bash
brick build examples/hello.brc -o hello
./hello
```

The `brick build` command handles the full pipeline:

1. Compiles `.brc` to C
2. Links the runtime (block memory allocator, I/O, hot reload)
3. Runs `gcc -O3` to produce a standalone binary

### Compile to C only

```bash
brick examples/hello.brc -o hello.c
```

> Useful if you want to inspect the generated C code.

### Release mode (no tracking overhead)

```bash
brick build examples/hello.brc --release -o hello
./hello
```

> Omit tracking overhead for maximum performance (no visualizer support).

## Run Tests

```bash
scons test                # testes unitários / unit tests
tests/test_integration.sh # testes de integração (.brc -> compila -> roda) / integration tests (.brc -> compile -> run)
```

## Visualize Memory

```bash
brick --visualize examples/hello.brc   # compila, roda e mostra TUI ao vivo / compile, run, show live TUI
brick --attach <pid>                  # conecta visualizador a processo rodando / attach visualizer to running process
```

## Core Concepts

1. **Everything in blocks**: your memory lives in blocks you declare
2. **No stack**: zero variables on the C stack (everything goes to blocks)
3. **Bump allocator**: super fast allocation (just advances a pointer)
4. **Reset, not free**: clears the entire block, never individual objects
5. **Hot reload**: swap code without stopping the program
6. **Fixed-width types**: i8/i16/i32/i64, u8/u16/u32/u64, f32/f64, usize/isize
7. **#line directives**: debug in the original .brc code, not the generated C

## Project Structure

| Directory | Contents |
|-----------|----------|
| `src/` | Compiler (Lexer, Parser, Codegen) in C++20 |
| `runtime/` | Block memory allocator + hot reload + IO (C) |
| `visualizer/` | ncurses TUI for live memory visualization |
| `debugger/` | GDB pretty-printers and custom commands (Python) |
| `vscode-ext/` | VS Code extension (highlight, LSP, memory view) |
| `tests/` | Unit and integration tests |
| `examples/` | Example .brc programs |
| `docs/` | GitHub Pages site |
| `wiki/` | GitHub Wiki content |
| `tasks/` | Development task breakdown (01-11) |
| `benchmarks/` | Performance benchmarks |

## Opening a Task (for contributors)

> Each folder in `tasks/` has a `run.sh` that opens opencode focused on that task.

> Each task has `AGENTS.md` (instructions for the AI) and `STATE.md` (where it stopped).

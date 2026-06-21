# Welcome to Meta-C

**Meta-C** is a programming language that compiles to C. It focuses on **maximum performance**, **explicit block-based memory management**, and **native hot reload**. Think of it as a high-level language with zero-cost abstractions — you get OOP syntax like GDScript but compile down to pure, readable C code that runs at native speed.

## Philosophy

- **Performance first**: Bump allocator, no exceptions, no RTTI, no garbage collector. You control memory.
- **Explicit memory**: Everything lives in user-managed blocks. No stack for user data, no individual `free()`, no memory leaks.
- **Hot reload built-in**: Swap code at runtime via `dlopen` + `inotify`. No external tools needed.
- **Debuggable**: Generated C code has `#line` directives pointing back to your `.mc` source. GDB shows your original Meta-C code.
- **Readable C output**: The generated C code is clean and readable — you can inspect it, learn from it, or continue development in C if needed.

## Quick Links

| Page | Description |
|------|-------------|
| [Getting Started](Getting-Started) | Install, build, compile, run, debug your first program |
| [Language Reference](Language-Reference) | Complete syntax, types, blocks, structs, functions, control flow, operators, packages, error handling |
| [Memory Blocks](Memory-Blocks) | Deep dive into block memory: declaration, allocation, scope, reset, best practices, performance |
| [Hot Reload](Hot-Reload) | Architecture, usage, and internals of the hot reload system |
| [Performance](Performance) | Bump allocator benchmarks, compiler optimizations, comparison vs malloc |
| [Architecture](Architecture) | Compiler pipeline (Lexer → Parser → Codegen), runtime, visualizer, debugger |
| [Contributing](Contributing) | How to contribute: setup, conventions, testing, build system, PR workflow |
| [VS Code Extension](VS-Code-Extension) | Syntax highlighting, LSP, debugger integration, Memory Webview |
| [FAQ](FAQ) | Frequently asked questions |

## A Taste of Meta-C

```meta-c
package HELLO

using IO

block global = 64MB
block temp   = 8MB

struct Greeter {
    String message

    fn Greeter(String msg) {
        message = msg
    }

    fn greet() {
        print(message)
    }
}

fn main() {
    print("Hello from Meta-C!")
    print(42)
    print(3.14)
    print(true)

    Greeter g = Greeter("Hello World!") @temp
    g.greet()

    print("done with {0} blocks", 2)

    temp.reset()
    global.reset()
}
```

## Project Structure

```
meta-c/
├── src/              → Compiler (C++20) — Lexer, Parser, Codegen
├── runtime/          → Runtime (C) — Block allocator, I/O, Hot Reload
├── visualizer/       → TUI memory visualizer (ncurses, C++)
├── debugger/         → GDB pretty-printers + custom commands (Python)
├── vscode-ext/       → VS Code extension (highlight, LSP, debug webview)
├── tests/            → Unit and integration tests
├── examples/         → Example .mc code
├── docs/             → Documentation
├── benchmarks/       → Performance benchmarks
├── tasks/            → 10 opencode agents for AI-assisted development
├── SConstruct        → SCons build entry point
└── build/            → Build artifacts
```

## Stack

| Component | Language | Purpose |
|-----------|----------|---------|
| Compiler | C++20 | Lexer, Parser, Codegen — produces C code |
| Runtime | C (C11) | Block memory allocator, hot reload engine, I/O |
| Visualizer | C++ (ncurses) | Real-time TUI for memory block inspection |
| Debugger | Python (GDB) | Pretty-printers + custom commands for block debugging |
| VS Code | TypeScript/JSON | Extension with syntax highlighting, LSP, debug webview |
| Build | Python (SCons) | Build system with cross-compilation support |

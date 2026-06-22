# Welcome to Brick

**Brick** is a programming language that compiles to C. It focuses on **maximum performance**, **explicit block-based memory management**, and **native hot reload**. Think of it as a high-level language with zero-cost abstractions — you get OOP syntax like GDScript but compile down to pure, readable C code that runs at native speed.

## Philosophy

- **Performance first**: Bump allocator, fixed-width types, no exceptions, no RTTI, no garbage collector.
- **Explicit memory**: Everything lives in user-managed blocks. No stack for user data, no individual `free()`, no memory leaks.
- **Hot reload built-in**: Swap code at runtime via `dlopen` + `inotify`. No external tools needed.
- **Debuggable**: Generated C code has `#line` directives pointing back to your `.brc` source.
- **Fixed-width types**: i8/i16/i32/i64, u8/u16/u32/u64, f32/f64, usize/isize with compile-time overflow checking.

## Quick Links

| Page | Description |
|------|-------------|
| [Getting Started](Getting-Started) | Install, build, compile, run, debug your first program |
| [Language Reference](Language-Reference) | Complete syntax, types, blocks, structs, functions, control flow, operators, packages |
| [Memory Blocks](Memory-Blocks) | Deep dive into block memory: declaration, allocation, scope, reset, best practices |
| [Hot Reload](Hot-Reload) | Architecture, usage, and internals of the hot reload system |
| [Performance](Performance) | Bump allocator benchmarks, compiler optimizations, comparison vs malloc |
| [Architecture](Architecture) | Compiler pipeline (Lexer → Parser → Codegen), runtime, visualizer, debugger |
| [Contributing](Contributing) | How to contribute: setup, conventions, testing, build system, PR workflow |
| [VS Code Extension](VS-Code-Extension) | Syntax highlighting, LSP, debugger integration, Memory Webview |
| [FAQ](FAQ) | Frequently asked questions |

## A Taste of Brick

```brick
package DEMO

using IO

interface Drawable {
    fn draw()
}

struct Circle {
    u32 id
    String name
    f32 radius

    fn Circle(u32 i, String n, f32 r) {
        id = i
        name = n
        radius = r
    }

    fn draw() {
        print("Circle #{0} radius={1}", id, radius)
    }
}

fn main() {
    Circle c = Circle(1u32, "Small", 5.0f32) @global
    c.draw()
}
```

## Quick Start

```bash
git clone https://github.com/nonunknown777/brick.git
cd brick
scons
brick run examples/hello.brc
```

## Project Structure

```
brick/
├── src/              → Compiler (C++20) — Lexer, Parser, Codegen
├── runtime/          → Runtime (C) — Block allocator, I/O, Hot Reload
├── visualizer/       → TUI memory visualizer (ncurses, C++)
├── debugger/         → GDB pretty-printers + custom commands (Python)
├── vscode-ext/       → VS Code extension (highlight, LSP, debug webview)
├── tests/            → Unit and integration tests
├── examples/         → Example .brc code
├── docs/             → GitHub Pages site
├── benchmarks/       → Performance benchmarks
├── tasks/            → 11 development tasks (AI-assisted)
├── SConstruct        → SCons build entry point
└── build/            → Build artifacts
```

## Stack

| Component | Language | Purpose |
|-----------|----------|---------|
| Compiler | C++20 | Lexer, Parser, Codegen — produces C code |
| Runtime | C (C11) | Block memory allocator, hot reload engine, I/O |
| Visualizer | C++ (ncurses) | Real-time TUI for memory block inspection |
| Debugger | Python (GDB) | Pretty-printers + custom commands |
| VS Code | TypeScript/JSON | Extension with syntax highlighting, LSP, debug webview |
| Build | Python (SCons) | Build system with cross-compilation support |

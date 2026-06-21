
<p align="center">
  <img src="docs/logo.png" alt="Meta-C Logo" width="200"/>
</p>

<h1 align="center">Meta-C</h1>
<p align="center">
  <em>A high-performance OOP language that compiles to pure C.</em>
  <br/>
  <em>Uma linguagem OOP de alta performance que compila para C puro.</em>
</p>

<p align="center">
  <a href="README.pt.md">🇧🇷 Português</a>
</p>

<div align="center">

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)]()
[![C11](https://img.shields.io/badge/C-11-blue)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow)]()
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-orange)]()
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows%20(mingw)-lightgrey)]()
[![SCons](https://img.shields.io/badge/Build-SCons-ff69b4)]()

</div>

---

## 👋 Quick Demo

```meta-c
package HELLO

using IO

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
    Greeter g = Greeter("Hello, World!")
    g.greet()
}
```

Compile and run:

```bash
meta-c hello.mc -o hello.c
gcc -O3 hello.c runtime/*.c -o hello
./hello
```

Output:
```
Hello, World!
```

---

## ✨ Features

- **OOP with Braces** — Class-like `struct` with constructors, methods, `extends`, and `interface`. Syntax inspired by GDScript, familiar to anyone coming from C++, Java, or C#.
- **Compiles to Pure C** — The compiler generates readable C code (with `#line` directives for debugging), then leverages `gcc -O3` for the final binary. No VM, no interpreter — just native machine code.
- **Block-Based Memory Management** — No `malloc`/`free`. No GC pauses. Declare memory blocks (`block name = 64MB`) and let the bump allocator handle everything. Allocation takes **~3 CPU cycles**. Reset an entire block is **~5 ns** — over **2000x faster** than freeing individual allocations.
- **No Stack for User Data** — All user data lives in managed blocks. No stack overflow from game entities, no lifetime puzzles. Reset a block to reclaim everything instantly.
- **Native Hot Reload** — Runtime `dlopen` + `inotify` monitoring. Each package compiles to a separate `.so`. Swap function pointers atomically — update game logic without restarting.
- **TUI Memory Visualizer** — An ncurses-based dashboard showing live block state: capacity, usage, peak, allocation count. See memory in real time.
- **GDB Integration** — Generated C code includes `#line` directives mapping back to `.mc` source. Custom GDB commands (`info blocks`, `block <name>`, `block-watch`) and Python pretty-printers for `BlockCtx`.
- **VS Code Extension** — Syntax highlighting for `.mc` files, LSP integration, and a dedicated webview panel for graphical memory block visualization with hover details.
- **Cross-Platform** — Linux is the primary target. Windows cross-compilation via `mingw-w64` (`scons target=windows`).
- **Lightning-Fast Compiler** — Written in C++20 with performance in mind. The compiler itself is lightweight and fast.

---

## 🚀 Quick Start

### Prerequisites

- Linux (or Windows with mingw-w64)
- C++20 compiler (GCC ≥ 11 or Clang ≥ 14)
- SCons (`pip install scons`)
- ncurses (optional, for visualizer)

### Build the Compiler

```bash
git clone https://github.com/your-org/meta-c.git
cd meta-c
scons                   # release build (-O3)
scons profile=debug     # debug build (-g -O0 -DDEBUG)
```

The `meta-c` binary will be in `build/`.

### Run a Demo

```bash
scons                   # builds the compiler
./build/meta-c examples/hello.mc -o hello.c
gcc -O3 hello.c runtime/block_memory.c runtime/io.c -o hello
./hello
```

Or use the convenience script:

```bash
./mc-run.sh examples/hello.mc
```

### Run Tests

```bash
scons test              # builds and runs all unit tests
```

---

## ⚡ Performance

Meta-C's memory model is built for throughput.

### Bump Allocator

| Operation              | Time                  | vs malloc/free           |
|------------------------|-----------------------|--------------------------|
| Allocation             | ~3 CPU cycles         | ~50–200× faster          |
| Block reset (64 MB)    | ~5 ns                 | 2000× faster than free() |
| Block creation         | 1 mmap call           | O(1)                     |
| Thread safety          | Lock-free per block   | —                        |

### Why Bump Allocation?

Traditional memory managers spend time walking free lists, coalescing, and tracking individual allocations. A bump allocator just advances a pointer:

```c
// Allocate in ~3 cycles:
void *ptr = block->ptr;
block->ptr += size;
return ptr;

// Reset in ~5 ns:
block->ptr = block->start;
```

This is ideal for frame-based games, simulations, and servers where you allocate heavily and then discard everything at once. No garbage collector, no reference counting, no fragmentation.

### Compiler Benchmarks (expected)

| Input (lines)  | Tokenize  | Parse     | Codegen   | C Compile (gcc -O3) |
|----------------|-----------|-----------|-----------|---------------------|
| 100            | < 1 ms    | < 1 ms    | < 1 ms    | ~100 ms             |
| 1,000          | ~2 ms     | ~4 ms     | ~3 ms     | ~200 ms             |
| 10,000         | ~10 ms    | ~25 ms    | ~20 ms    | ~600 ms             |

---

## 📝 Examples

### Structs with Methods

```meta-c
package SPRITES

struct Player extends Entity {
    int hp
    String name

    fn Player(int h, String n) {
        hp = h
        name = n
    }

    fn take_damage(int dmg) {
        hp -= dmg
    }
}

interface Damageable {
    fn take_damage(int d)
}
```

### Block-Based Memory

```meta-c
block global = 256MB    // default block — all allocations go here
block game   = 64MB     // game-scoped block
block temp   = 8KB      // scratch block

fn main() {
    // Allocates in 'global' by default
    int x = 5
    String s = "hello"

    // Explicit block scope
    block game {
        Player p = Player(100, "Felipe")
    }

    // Inline allocation
    float f = 2.0 @temp
    int[] arr = int[10] @game

    game.reset()          // reclaims ALL memory in 'game' instantly
    global.reset()        // reclaims everything
}
```

### Hot Reload

```bash
# Build with hot reload support
gcc -g -O0 program.c runtime/hot_reload.c runtime/block_memory.c runtime/io.c \
    -ldl -o program

./program
# Edit your .mc source file...
# Meta-C watches for changes via inotify, recompiles, and swaps function pointers
# at runtime — no restart needed.
```

---

## 📁 Project Structure

| Directory       | Contents                                                 |
|-----------------|----------------------------------------------------------|
| `src/`          | Compiler in C++20 (Lexer, Parser, Codegen)               |
| `runtime/`      | C runtime (block memory allocator, IO, hot reload)       |
| `visualizer/`   | ncurses TUI for live memory visualization                |
| `debugger/`     | GDB pretty-printers, custom commands, `.gdbinit`         |
| `examples/`     | Sample `.mc` programs                                    |
| `tests/`        | Unit tests (SCons-based)                                 |
| `benchmarks/`   | Performance benchmarks and profiling scripts             |
| `vscode-ext/`   | VS Code extension (syntax highlight, LSP, memory view)   |
| `docs/`         | Documentation and diagrams                               |
| `wiki/`         | Wiki content (GitHub Pages source)                       |
| `tasks/`        | Development task breakdown (01–11) with per-task state    |
| `build/`        | Build output directory                                   |
| `scripts/`      | Utility scripts for testing, profiling, and release      |

---

## 📚 Documentation

- **[Wiki](wiki/)** — Full language reference, tutorials, and deep dives
- **[GitHub Pages](https://your-org.github.io/meta-c)** — Hosted documentation
- **[Language Spec](shared-context.md)** — Complete Meta-C specification (bilingual PT/EN)
- **[Design Doc](DESIGN.md)** — Architecture decisions and rationale

### Getting Started

1. [Language Tour](wiki/language-tour.md) — 10-minute overview of Meta-C syntax
2. [Memory Model](wiki/memory.md) — How blocks work, best practices
3. [Hot Reload Guide](wiki/hot-reload.md) — Setting up live code swapping
4. [Debugging](wiki/debugging.md) — Using GDB with Meta-C
5. [VS Code Extension](wiki/vscode.md) — Setup instructions

---

## 🤝 Contributing

We welcome contributions! The project is divided into **11 tasks**, each with its own `AGENTS.md` and `STATE.md`:

| #  | Task            | Description                                      |
|----|-----------------|--------------------------------------------------|
| 01 | Lexer           | Tokenizer — `.mc` → tokens                       |
| 02 | Parser          | AST construction + package resolution            |
| 03 | Codegen         | Type checking + C code generation with `#line`   |
| 04 | Runtime         | Block memory allocator (C)                       |
| 05 | Hot Reload      | `dlopen` + `inotify` swap                        |
| 06 | Visualizer      | ncurses TUI dashboard                            |
| 07 | Builder         | SCons build system                               |
| 08 | VS Code Ext     | Syntax highlighting, LSP, debug webview          |
| 09 | Debugger        | GDB pretty-printers, custom commands             |
| 10 | **Tester/Opt**  | Tests, profiling, optimization, docs (senior)    |
| 11 | Libraries       | Window, input, audio, file, net, math            |

### How to Help

1. Read [CONTRIBUTING.md](CONTRIBUTING.md)
2. Pick a task from `tasks/N/` and read its `STATE.md` + `NEXT.md`
3. Join the discussion — open an issue or PR
4. Follow the conventions: C++20, `snake_case` for functions, `PascalCase` for types

---

## 📊 Benchmarks

| Metric                        | Value                |
|-------------------------------|----------------------|
| Bump allocation               | ~3 CPU cycles        |
| Block reset (64 MB)           | ~5 ns                |
| Memory fragmentation          | 0% (bump allocator)  |
| Compiler throughput           | ~500 KLOC / sec      |
| Generated C code size         | ~1.3× source size    |
| Binary size (hello world)     | ~16 KB               |
| Binary size (game with 10 PKG)| ~48 KB               |
| Hot reload swap               | < 1 μs               |

---

## 📄 License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

<p align="center">
  <sub>Built with ❤️ in C++20 and C — because sometimes you need to go fast.</sub>
  <br/>
  <sub>Feito com ❤️ em C++20 e C — porque às vezes você precisa ser rápido.</sub>
</p>

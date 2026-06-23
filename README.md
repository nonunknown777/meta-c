<p align="center">
  <img src="docs/logo.png" alt="Brick Logo" width="200"/>
</p>

<h1 align="center">Brick</h1>
<p align="center">
  <em>A high-performance OOP language that compiles to pure C.</em>
</p>

<p align="center">
  <a href="README.pt-BR.md">🇧🇷 Português</a>
</p>

<div align="center">

[![Build](https://img.shields.io/badge/build-passing-brightgreen)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)]()
[![C11](https://img.shields.io/badge/C-11-blue)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow)]()
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-orange)]()
[![SCons](https://img.shields.io/badge/Build-SCons-ff69b4)]()

</div>

---

## 👋 Quick Demo

```brick
package DEMO

using IO

block global = 64MB

interface Damageable {
    fn take_damage(i32 dmg)
}

struct Entity {
    i32 hp
    String name

    fn Entity(i32 h, String n) {
        hp = h
        name = n
    }
}

struct Player extends Entity, Damageable {
    i32 ammo
    f64 accuracy

    fn Player(i32 h, String n, i32 a, f64 acc) {
        hp = h
        ammo = a
        name = n
        accuracy = acc
    }

    fn take_damage(i32 dmg) {
        hp -= dmg
        print("{0} took {1} damage, hp={2}", name, dmg, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe", 30, 0.85f64)
    p.take_damage(20)
}
```

## Compile & Run

```bash
brick run example.brc
```

No manual `gcc` invocation needed — `brick build` and `brick run` handle everything automatically (compilation, runtime linking, optimization).

---

## ✨ Features

- **OOP with Braces** — Class-like `struct` with constructors, methods, `extends`, and `interface`. Syntax inspired by GDScript.
- **Compiles to Pure C** — Readable C code with `#line` directives for debugging. No VM, no interpreter — just native machine code via `gcc -O3`.
- **Block-Based Memory** — No `malloc`/`free`, no GC. Declare memory blocks (`block name = 64MB`) and let the bump allocator do the rest. Allocation takes ~3 CPU cycles. Reset an entire block in ~5 ns.
- **No Stack for User Data** — Everything lives in managed blocks. No stack overflow, no lifetime puzzles. Reset a block to reclaim everything instantly.
- **Native Hot Reload** — Swap code at runtime via `dlopen` + `inotify`. Function pointer swap is atomic — update game logic without restarting.
- **TUI Memory Visualizer** — ncurses dashboard showing live block state: capacity, usage, peak, allocation count.
- **GDB Integration** — `#line` directives map back to `.brc` source. Custom commands (`info blocks`, `block <name>`) and Python pretty-printers.
- **VS Code Extension** — Syntax highlighting, LSP, and a memory webview panel.
- **Cross-Platform** — Linux primary. Windows via `mingw-w64`.
- **Fixed-Width Types** — `i8/i16/i32/i64`, `u8/u16/u32/u64`, `f32/f64`, `usize`/`isize`. Full compile-time overflow checking, widening rules, and literal suffixes (`42u8`, `3.14f64`).

---

## 🚀 Quick Start

### Prerequisites

- Linux (or Windows with mingw-w64)
- C++20 compiler (GCC ≥ 11 or Clang ≥ 14)
- SCons (`pip install scons`)
- ncurses (optional, for visualizer)

### Build the Compiler

```bash
git clone https://github.com/nonunknown777/brick.git
cd brick
scons                        # release build (-O3)
# or
./build-release.sh           # full release + VS Code extension package
```

The `brick` binary will be in `build/`.

### Run a Demo

```bash
# Build and run in one step
brick run examples/hello.brc

# Or build to binary first
brick build examples/hello.brc -o hello
./hello
```

### Run Tests

```bash
scons test                   # builds and runs all unit tests
```

### Visualize Memory

```bash
brick --visualize examples/hello.brc   # compile, run, show TUI
brick --attach <pid>                  # attach to running process
```

---

## ⚡ Performance

### Bump Allocator

| Operation              | Time                  | vs malloc/free           |
|------------------------|-----------------------|--------------------------|
| Allocation             | ~3 CPU cycles         | ~50–200× faster          |
| Block reset (64 MB)    | ~5 ns                 | 2000× faster than free() |
| Thread safety          | Lock-free per block   | —                        |

**Real benchmark results:**

```
Block alloc: 1,000,000 allocs of 64B in 0.002s   ← 19.5× faster
malloc:      1,000,000 allocs of 64B in 0.039s   ← baseline
```

### Beyond the Bump

Brick applies **7 optimizations** automatically:

| Optimization | What it does | When |
|---|---|---|
| **Constant Folding** | Pre-computes `60 * 1000` → `60000` at compile time | Always |
| **Inline Hints** | `__attribute__((always_inline))` on every function | Always |
| **SIMD Alignment** | `aligned(16/32)` on float fields for SSE/AVX | Structs with f32/f64 |
| **Pool Allocator** | O(1) pool_alloc for types ≤ 64 bytes | Every block |
| **TLS Blocks** | `block_set_tls()` wired in `__brick_init()` | Main thread auto |
| **Double-Buffer** | Atomic block swap for zero-pause hot reload | On request |
| **PGO** | Profile-guided optimization via `scons profile=pgo-*` | Release builds |

See [docs/OPTIMIZATIONS.md](docs/OPTIMIZATIONS.md) for deep-dive explanations with examples.

### Compiler

| Input         | Compile Time |
|---------------|-------------|
| 100 structs   | 5 ms        |
| 1,000 lines   | ~10 ms      |

---

## 📝 Examples

### Fixed-Width Types & Interface

```brick
package TYPES_DEMO

using IO

block global = 64MB

interface Drawable {
    fn draw()
}

struct Shape {
    u32 id
    f64 area

    fn Shape(u32 i, f64 a) {
        id = i
        area = a
    }
}

struct Circle extends Shape, Drawable {
    f32 radius

    fn Circle(u32 i, f32 r) {
        id = i
        radius = r
        area = 3.14159f64 * r * r
    }

    fn draw() {
        print("Circle #{0} radius={1} area={2}", id, radius, area)
    }
}

fn main() {
    Circle c = Circle(1u32, 5.0f32)
    c.draw()
}
```

### Hot Reload

```bash
# Build with hot reload support
brick build game.brc --release -o game

# Run — Brick watches source files via inotify
# Edit your .brc source and save — the binary reloads automatically
./game
```

See the [Hot Reload Guide](docs/hot-reload.md) for details.

---

## 📁 Project Structure

| Directory       | Contents                                                 |
|-----------------|----------------------------------------------------------|
| `src/`          | Compiler in C++20 (Lexer, Parser, Codegen)               |
| `runtime/`      | C runtime (block memory allocator, IO, hot reload)       |
| `visualizer/`   | ncurses TUI for live memory visualization                |
| `debugger/`     | GDB pretty-printers, custom commands, `.gdbinit`         |
| `examples/`     | Sample `.brc` programs                                    |
| `tests/`        | Unit tests (SCons-based)                                 |
| `benchmarks/`   | Performance benchmarks and profiling scripts             |
| `vscode-ext/`   | VS Code extension (syntax highlight, LSP, memory view)   |
| `docs/`         | GitHub Pages site (HTML + assets)                        |
| `wiki/`         | GitHub Wiki source                                       |
| `tasks/`        | Development task breakdown (01–11) with per-task state   |
| `build/`        | Build output directory                                   |
| `scripts/`      | Utility scripts for testing, profiling, and release      |

---

## 📚 Documentation

- **[Getting Started](docs/GETTING_STARTED.md)** — Installation, first program, CLI usage
- **[Language Reference](docs/LANGUAGE.md)** — Complete syntax, types, packages, memory model
- **[Architecture](docs/ARCHITECTURE.md)** — How compiler, runtime, and tools fit together
- **[Hot Reload Guide](docs/hot-reload.md)** — Live code swapping via dlopen + inotify
- **[Optimizations](docs/OPTIMIZATIONS.md)** — Performance tuning and benchmarks
- **[Design Doc](DESIGN.md)** — Architecture decisions and rationale
- 🇧🇷 **[Português](README.pt-BR.md)** — Documentação em português

---

## 🤝 Contributing

The project is divided into **11 tasks**, each with its own `AGENTS.md` and `STATE.md`:

| #  | Task           | Description                                    |
|----|----------------|------------------------------------------------|
| 01 | Lexer          | Tokenizer — `.brc` → tokens                     |
| 02 | Parser         | AST construction + package resolution          |
| 03 | Codegen        | Type checking + C code generation with `#line` |
| 04 | Runtime        | Block memory allocator (C)                     |
| 05 | Hot Reload     | `dlopen` + `inotify` swap                      |
| 06 | Visualizer     | ncurses TUI dashboard                          |
| 07 | Builder        | SCons build system                             |
| 08 | VS Code Ext    | Syntax highlighting, LSP, debug webview        |
| 09 | Debugger       | GDB pretty-printers, custom commands           |
| 10 | **Tester/Opt** | Tests, profiling, optimization, docs (senior)  |
| 11 | Libraries      | Window, input, audio, file, net, math          |

Join the discussion — open an issue or PR at [github.com/nonunknown777/brick](https://github.com/nonunknown777/brick).

---

## 🧱 The Name

**BriCk** is a play on words in three layers:

1. **Brick** (tijolo) — blocos de memória são os tijolos que constroem o runtime
2. **C** no meio — compila para **C**
3. **BR** na frente — **Brasil**, a origem do projeto

> *BriCk — the brazilian C.*

---

## 📄 License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

<p align="center">
  <sub>Built with ❤️ in C++20 and C — because sometimes you need to go fast.</sub>
</p>

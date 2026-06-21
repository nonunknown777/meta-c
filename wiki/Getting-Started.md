# Getting Started with Meta-C

Quick guide to install, build, compile, run, and debug Meta-C programs.

---

## Prerequisites

| Dependency | Purpose | Install (Arch Linux) | Install (Ubuntu/Debian) |
|------------|---------|---------------------|-------------------------|
| **g++** (C++20) | Compile the Meta-C compiler | `sudo pacman -S gcc` | `sudo apt install g++` |
| **gcc** | Compile generated C code | `sudo pacman -S gcc` | `sudo apt install gcc` |
| **SCons** | Build system | `sudo pacman -S scons` | `sudo apt install scons` |
| **ncurses** | TUI visualizer | `sudo pacman -S ncurses` | `sudo apt install libncurses-dev` |
| **dl** (ld) | Hot reload | (built-in with glibc) | (built-in with glibc) |
| **pthread** | Hot reload threads | (built-in with glibc) | (built-in with glibc) |
| **X11** (optional) | Window library | `sudo pacman -S libx11` | `sudo apt install libx11-dev` |

Meta-C is primarily developed on **Linux (Arch)**, but any Linux distribution with these packages will work.

---

## Clone & Build

```bash
# Clone the repository
git clone <repo-url> meta-c
cd meta-c

# Build the Meta-C compiler (release mode — default)
scons

# Build with debug symbols
scons profile=debug

# Build with sanitizers (for development)
scons profile=sanitize

# Cross-compile to Windows (requires mingw-w64)
scons target=windows
```

After building, the `meta-c` compiler binary will be at:

```
build/meta-c
```

### Build Profiles

| Profile | Flags | Use Case |
|---------|-------|----------|
| `release` | `-O3 -std=c++20` | Production use (default) |
| `debug` | `-g -O0 -DDEBUG` | Compiler development |
| `sanitize` | `-g -O1 -fsanitize=address,undefined` | Finding bugs |

---

## Compile and Run Your First Program

### Step 1: Create a `.mc` file

Save this as `hello.mc`:

```meta-c
package HELLO

using IO

block global = 64MB
block temp = 8MB

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

### Step 2: Compile `.mc` → `.c`

```bash
# Create output directory
mkdir -p build

# Produce C code
build/meta-c hello.mc -o build/hello.c
```

### Step 3: Compile `.c` → Binary

```bash
# Release build (maximum performance)
gcc -O3 build/hello.c runtime/block_memory.c runtime/io.c -o build/hello -ldl

# Debug build (with hot reload + GDB support)
gcc -g build/hello.c runtime/block_memory.c runtime/hot_reload.c runtime/io.c -o build/hello -ldl
```

### Step 4: Run

```bash
./build/hello
```

Expected output:

```
Hello from Meta-C!
42
3.140000
true
Hello World!
done with 2 blocks
```

---

## Two-in-One Commands: `build` and `run`

The Meta-C compiler also provides convenience subcommands:

```bash
# Build everything in one step (.mc → .c → gcc → binary)
build/meta-c build hello.mc -o build/hello

# Build and run in one step (creates temp binary, runs it, cleans up)
build/meta-c run hello.mc

# Release mode (no tracking overhead, max performance)
build/meta-c build hello.mc -o build/hello --release
```

These commands:
1. Compile `.mc` → `.c`
2. Invoke `gcc` with the correct runtime files
3. Produce a final binary (for `build`) or run it directly (for `run`)

---

## Debugging

### Using GDB with Meta-C Support

```bash
# Compile C with debug symbols
gcc -g build/hello.c runtime/block_memory.c runtime/hot_reload.c runtime/io.c -o build/hello -ldl

# Launch GDB
gdb ./build/hello
```

The `.gdbinit` in the project automatically loads Meta-C's debug support:

**Custom GDB Commands:**

```
(gdb) info blocks           # List all memory blocks with usage stats
(gdb) block <name>          # Show details of a specific block
(gdb) block-watch <name>    # Set breakpoint on block allocation
(gdb) blocks-list           # List block names (for scripting)
(gdb) ib                    # Alias for "info blocks"
```

**Pretty-Printers:**

- `BlockCtx*`: Shows capacity, usage %, allocation count, and a visual bar
- `MetaCString`: Shows string content with length
- Block-allocated pointers: Show `@blockname+offset` with block fullness

Because the generated C code includes `#line` directives, GDB will display your original `.mc` source code (not the generated C) when stepping through the program.

### VS Code Debugging

1. Open the project in VS Code
2. Install the Meta-C extension (`vscode-ext/`)
3. Press F5 with a `.mc` file open
4. Use the "Meta-C Memory" webview to see memory blocks visually

See [VS Code Extension](VS-Code-Extension) for details.

---

## Running Tests

```bash
# Unit tests (lexer, parser, codegen, runtime, hot reload)
scons test

# Integration tests (compile .mc → run binary → verify output)
tests/test_integration.sh

# Benchmarks (compilation time, allocator performance)
benchmarks/run_benchmarks.sh
```

---

## TUI Memory Visualizer

The ncurses-based visualizer shows memory blocks in real time:

```bash
# Run standalone
build/meta-c --visualize

# Attach to a running Meta-C process
build/meta-c --attach <pid>
```

---

## LSP Mode

The compiler supports LSP (Language Server Protocol) for editor integration:

```bash
build/meta-c input.mc --lsp
```

This outputs JSON with syntax errors, tokens, and symbol information.

---

## Project Layout

```
meta-c/
├── src/              → Compiler
│   ├── lexer/        → .mc → tokens
│   ├── parser/       → tokens → AST + package resolution
│   └── codegen/      → AST → C code + type checking
├── runtime/          → C runtime library
│   ├── block_memory.c  → Bump allocator
│   ├── io.c           → Print wrappers
│   └── hot_reload.c   → dlopen + inotify
├── visualizer/       → ncurses TUI
├── debugger/         → GDB Python scripts
├── vscode-ext/       → VS Code extension
├── tests/            → Test suites
├── examples/         → Example .mc code
├── benchmarks/       → Performance benchmarks
└── docs/             → Documentation
```

---

## Quick Reference

```bash
# Build compiler
scons

# Compile .mc → .c
build/meta-c input.mc -o output.c

# Compile .mc → binary (one step)
build/meta-c build input.mc -o output

# Compile and run
build/meta-c run input.mc

# Debug
gcc -g output.c runtime/*.c -o program -ldl && gdb ./program

# Test
scons test
tests/test_integration.sh

# Visualize
build/meta-c --visualize
```

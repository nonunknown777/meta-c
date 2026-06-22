# Getting Started with Brick

Quick guide to install, build, compile, run, and debug Brick programs.

## Prerequisites

| Dependency | Purpose | Install (Arch Linux) | Install (Ubuntu/Debian) |
|------------|---------|---------------------|-------------------------|
| **g++** (C++20) | Compile the Brick compiler | `sudo pacman -S gcc` | `sudo apt install g++` |
| **gcc** | Compile generated C code | `sudo pacman -S gcc` | `sudo apt install gcc` |
| **SCons** | Build system | `sudo pacman -S scons` | `sudo apt install scons` |
| **ncurses** | TUI visualizer | `sudo pacman -S ncurses` | `sudo apt install libncurses-dev` |
| **X11** (optional) | Window library | `sudo pacman -S libx11` | `sudo apt install libx11-dev` |

## Clone & Build

```bash
git clone https://github.com/nonunknown777/brick.git
cd brick
scons                     # release build (default)
scons profile=debug       # debug build
scons profile=sanitize    # sanitizers (for finding bugs)
```

The `brick` compiler will be at `build/brick`.

## Compile and Run Your First Program

### One-step (build + run)

```bash
build/brick run examples/hello.brc
```

### Build to binary

```bash
build/brick build examples/hello.brc -o hello
./hello
```

### Expected output

```
Hello from Brick!
42
3.140000
true
Hello World!
done with 2 blocks
```

## Two-in-One Commands

The `brick` CLI provides convenience subcommands:

```bash
# Build everything in one step (.brc → .c → gcc → binary)
build/brick build hello.brc -o hello

# Build and run in one step
build/brick run hello.brc

# Release mode (no tracking overhead, max performance)
build/brick build hello.brc --release -o hello
```

## Debugging

```bash
# Build with debug symbols
build/brick build hello.brc -o hello   # always includes -g

# Launch GDB
gdb ./hello
```

The `.gdbinit` in the project automatically loads Brick's debug support.

**Custom GDB Commands:**

```
(gdb) info blocks           # List all memory blocks with usage stats
(gdb) block <name>          # Show details of a specific block
(gdb) block-watch <name>    # Set breakpoint on block allocation
(gdb) ib                    # Alias for "info blocks"
```

Because the generated C code includes `#line` directives, GDB will display your original `.brc` source code.

## Running Tests

```bash
scons test                    # unit tests
tests/test_integration.sh     # integration tests (.brc → compile → run)
benchmarks/run_benchmarks.sh  # performance benchmarks
```

## TUI Memory Visualizer

```bash
build/brick --visualize examples/hello.brc   # compile, run, show TUI
build/brick --attach <pid>                  # attach to running process
```

## Quick Reference

```bash
scons                          # Build compiler
build/brick run input.brc      # Compile and run
build/brick build input.brc -o out  # Build to binary
build/brick input.brc -o out.c # Compile to C only
scons test                     # Run tests
build/brick --visualize file  # Compile + run + visualize
```

## Project Layout

```
brick/
├── src/              → Compiler (C++20)
├── runtime/          → C runtime (block alloc, IO, hot reload)
├── visualizer/       → ncurses TUI
├── debugger/         → GDB Python scripts
├── vscode-ext/       → VS Code extension
├── tests/            → Test suites
├── examples/         → Example .brc code
├── benchmarks/       → Performance benchmarks
└── docs/             → GitHub Pages site
```

# Frequently Asked Questions

## General

### What is Brick?

Brick is a programming language that compiles to C. It gives you OOP syntax (structs with methods, inheritance, interfaces) and explicit block-based memory management, then compiles down to pure C code that compiles with `gcc -O3`. It's designed for maximum performance with native hot reload support.

### Why compile to C instead of LLVM IR or machine code?

1. **Portability**: C is everywhere. Any platform with a C compiler can run Brick programs.
2. **Optimization**: Let gcc/clang do the heavy lifting — they've been tuned for decades.
3. **Readability**: The generated C code is clean and readable. You can inspect what your `.brc` code becomes.
4. **Interop**: Calling C libraries is trivial — the output is C.

### Is Brick production-ready?

Brick is in active development (v0.1). It works for learning, prototyping, and small-to-medium programs. The compiler, runtime, and tools are functional, but expect rough edges and missing features.

### What platforms are supported?

Currently **Linux only**. The runtime uses POSIX APIs (`dlopen`, `inotify`, `pthread`). Cross-compilation to Windows is partially supported via mingw-w64 (no hot reload on Windows yet).

### How is Brick different from C++?

| Aspect | Brick | C++ |
|--------|--------|-----|
| Memory | Block-based (bump alloc) | Stack + heap (malloc/new) |
| OOP | Structs with methods | Classes with virtual functions |
| Compilation | Generates C, then gcc | Generates machine code directly |
| Hot Reload | Built-in (dlopen) | External tools needed |
| Exceptions | None (error() panic) | try/catch/throw |
| Standard Library | Minimal (I/O, blocks) | STL (huge) |
| Compile times | <50ms for 1000 lines | Can be slow (templates, headers) |

---

## Language

### Is Brick garbage collected?

**No**. Memory is managed explicitly through blocks. You declare blocks, allocate into them, and reset entire blocks when done. There is no garbage collector, no reference counting, and no individual free. This gives predictable, maximum performance.

### How do I free individual objects?

You don't. Brick uses a bump allocator — all objects in a block are freed together when you call `block.reset()`. Design your data to group objects by lifetime into the same block.

### Can I use stack variables?

**No**. All user data goes into blocks. The compiler uses an internal anonymous block for function parameters and return values, but you cannot declare stack variables. This avoids stack overflow for large data and keeps the memory model consistent.

### Does Brick have pointers?

Brick does not expose raw pointer syntax to the user. However, block-allocated variables are implemented as pointers in the generated C code. You can have cross-block references implicitly.

### Does Brick have arrays?

Yes, fixed-size arrays: `int[10] arr`. Dynamic arrays are not yet supported.

### Does Brick have strings?

Yes, `String` is a built-in type. It's a struct with a data pointer and length, allocated in blocks like any other data.

### Can I use C libraries from Brick?

Currently, Brick does not have a FFI (Foreign Function Interface). The workaround is to add C wrappers to the runtime and call them from the generated code. A proper FFI is planned for a future version.

### Why no `this` keyword?

Simplicity. Inside a method, field names are resolved to the struct's fields automatically. No `this->foo` or `self.foo` needed. This also prevents name shadowing.

### Why no try/catch?

Exceptions add runtime overhead (stack unwinding tables, RTTI). Brick's approach is to fail fast with `error("msg")` — the program panics and prints a message. This keeps performance predictable and code simple.

---

## Memory Blocks

### What happens if a block overflows?

The program panics with `error("block overflow: out of memory in block")`. There is no recovery — the program exits. Choose block sizes carefully based on your peak usage.

### Can I resize a block after creation?

Not yet. Block size is fixed at declaration. This is planned as a future feature.

### Is block allocation thread-safe?

By default, no — the block allocator has no locks. This gives maximum single-threaded performance. A thread-safe mode (atomic operations or per-thread blocks) is planned.

### Do I need to zero memory after reset?

No. `block.reset()` sets the bump pointer to 0 but does not zero the memory. The old data is simply overwritten on the next allocation. If you need to zero memory for security reasons, you can do it manually after reset.

### Can I have nested blocks inside each other's scope?

Yes. Block scopes (`block name: { }`) can be nested:

```brick
block outer {
    block inner {
        // both blocks accessible
    }
}
```

The compiler saves and restores the current block context at each level.

---

## Hot Reload

### Does hot reload work on Windows?

No, it uses Linux-specific APIs (`dlopen`, `inotify`). Windows support is not currently planned.

### Is hot reload safe?

The system is designed to be safe:
- Block allocations are frozen during the swap
- Function pointers are swapped atomically
- If the new `.so` fails to load, the old code is kept
- Struct layouts must be compatible (add fields at end)

### Can I hot reload data?

No — only code (functions) is hot-reloaded. Data in blocks persists across reloads. This is by design: you don't lose game state when reloading game logic.

### How fast is hot reload?

The swap itself is microseconds (just `dlopen` + atomic pointer swaps). The monitoring thread checks for file changes via `inotify`, which is instantaneous. The only delay is copying the `.so` file and calling `dlopen`.

---

## Performance

### How does the bump allocator compare to malloc?

The bump allocator is ~27x faster for allocation and ~2000x faster for bulk deallocation:

| Operation | malloc | Brick block |
|-----------|:------:|:------------:|
| Allocate 64 bytes | ~80 cycles | ~3 cycles |
| Deallocate 1M objects | ~2 seconds (free loop) | ~0.001 seconds (one reset) |
| Fragmentation | 10-30% | 0% |
| Per-allocation overhead | 8-16 bytes | 0 bytes |

### Is the generated C code slower than hand-written C?

Generally no, and it can be faster. The bump allocator is more efficient than typical `malloc` patterns, and the compiler generates clean C that `gcc -O3` optimizes well. The main performance difference is the lack of virtual dispatch (all calls are direct) and the absence of exception handling overhead.

### What about compile times?

Very fast. Brick has no template instantiation, no header inclusion, and no optimization passes. Target: <50ms for 1000 lines of `.brc`.

---

## Development

### How do I build the compiler?

```bash
git clone <repo> && cd brick
scons                    # release build
scons profile=debug      # debug build
```

See [Getting Started](Getting-Started) for details.

### How do I run tests?

```bash
scons test               # unit tests
tests/test_integration.sh # integration tests
benchmarks/run_benchmarks.sh  # performance benchmarks
```

### Can I contribute?

Absolutely! See the [Contributing](Contributing) guide for setup, conventions, and workflow.

### What should I work on?

Check the `tasks/` directory — each task has a `STATE.md` showing current progress and `NEXT.md` showing what needs to be done. Task 10 (Tester/Optimizer) coordinates progress across all tasks.

### Why SCons instead of CMake?

SCons is Python-based, which fits the project's philosophy of simplicity. It's more concise than CMake for this project's scale, and Python is easier for contributors to understand and modify.

### Does the project have a Code of Conduct?

Yes — be excellent to each other. The project is open to everyone regardless of background, experience level, or identity.

---

## Troubleshooting

### "block overflow" error

Your block is too small for the data you're allocating. Measure actual usage with `info blocks` in GDB or the TUI visualizer, then increase the block size.

### Compiler crashes with "unexpected character"

You might have a unsupported character in your source. Brick only supports ASCII. Check for non-ASCII characters, smart quotes, em dashes, etc.

### `print()` doesn't work

Did you add `using IO` at the top of your file? The `print()` function requires the IO package.

### GDB shows C code, not Brick code

Make sure:
1. You compiled with `gcc -g` (debug symbols)
2. The `.c` file was generated by the Brick compiler (has `#line` directives)
3. The `.brc` source file is in its original location

### "hr: symbol not found"

Your `.so` doesn't export the expected function names. Make sure the function names match between your registration code and the compiled `.so`.

### Visualizer shows no blocks

Make sure the program was compiled with `-DBRICK_TRACK_BLOCKS` (it's on by default in debug builds, off in `--release` mode).

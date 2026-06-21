# Performance

Meta-C is designed for **maximum performance** across all layers: the compiler, the generated code, and the runtime. This page documents the performance philosophy, optimizations, benchmarks, and comparison to traditional approaches.

---

## Performance Philosophy

Performance is engineered at three layers:

```
LAYER 1: COMPILER (C++20)
  ├─ Templates and constexpr for compile-time computation
  ├─ Recursive descent parser (O(n), no backtracking)
  ├─ AST nodes without heavy smart pointers
  ├─ Continuous stringstream (no multiple allocations)
  ├─ unordered_map for keyword lookup (O(1))
  └─ Runs at development time → not performance-critical for users

LAYER 2: GENERATED C CODE
  ├─ Pure, readable C — no hidden abstractions
  ├─ Flat structs (no hidden vtable)
  ├─ Direct function calls (no virtual dispatch)
  ├─ Direct assignment (no C++ construction/destruction)
  ├─ #line directives generate zero extra machine code
  └─ Compile with gcc/clang -O3 for full optimization

LAYER 3: RUNTIME (C)
  ├─ Bump allocator: allocation in ~3 CPU cycles
  ├─ block_reset: just `ctx->used = 0` (O(1))
  ├─ Zero-overhead features (no exceptions, no RTTI)
  ├─ No garbage collector (no "stop the world")
  └─ Configurable alignment for SIMD
```

---

## Bump Allocator: The Heart of Performance

### How It Works

Traditional `malloc` must search a free list for available space, which is O(n) and causes fragmentation. The bump allocator is drastically simpler:

```c
// Simplified bump allocator — actual code in runtime/block_memory.c
void* block_alloc(BlockCtx* ctx, size_t size) {
    size_t aligned = (ctx->used + alignment - 1) & ~(alignment - 1);
    if (aligned + size > ctx->capacity)
        error("block overflow");
    ctx->used = aligned + size;
    return ctx->data + aligned;  // ← just advance a pointer
}
```

**What the bump allocator avoids:**
- Free list traversal
- Memory coalescing
- Lock contention (single thread by default)
- Fragmentation tracking
- Metadata headers per allocation

### Benchmarks

```bash
# Run the benchmark suite
benchmarks/run_benchmarks.sh
```

Expected results:

```
Allocation: 1,000,000 allocs of 64 bytes
  Block alloc:   0.003s  (~3 cycles/allocation)
  malloc:        0.082s  (~80 cycles/allocation)
  → Bump allocator is ~27x faster

Reset: 100,000 resets + allocs
  Block reset:   0.001s  (~1 cycle/reset)
  free() loop:   2.100s  (~2000x slower)

Compilation: 100 structs
  Meta-C:        <50ms
```

### Why It's So Fast

| Factor | Why |
|--------|-----|
| No free search | Just advance a pointer — no list walk |
| No fragmentation | All allocations are contiguous — 0% waste |
| Cache friendly | Data is packed close together |
| No locks | Single-threaded by default (optional thread-safe) |
| Constant time | Both alloc and reset are O(1) |

---

## Compiler Optimizations

### Parser

The recursive descent parser is **O(n)** with no backtracking:

- Each token is consumed exactly once
- No speculative parsing
- No AST rewriting passes
- Keyword lookup via `std::unordered_map` (amortized O(1))

### Codegen

The code generator writes C directly with no intermediate representation:

- No optimization passes on AST
- No dead code elimination (let gcc handle that)
- No constant folding (planned for future)
- Stringstream output avoids repeated allocations
- `#line` directives add zero runtime cost

### Build

The SCons build system caches compilation artifacts:

```bash
.scons_cache/          # Cached object files
scons profile=release  # -O3 -std=c++20 (default)
scons profile=debug    # -g -O0 -DDEBUG
scons profile=sanitize # -g -O1 -fsanitize=address,undefined
```

---

## Optimizations in Generated Code

### Flat Structs (No Vtable)

Meta-C structs are pure C structs with no hidden vtable pointer:

```meta-c
struct Player {
    int hp
    String name
}
```

```c
// Generated C — just data, no extra fields
typedef struct Player {
    int64_t hp;
    MetaCString name;
} Player;
```

Methods become plain C functions:

```c
// method call: player.take_damage(10)
Player_take_damage(&player, 10);

// direct call, no virtual dispatch, no branch prediction failure
```

### No Hidden Abstractions

| C++ Feature | Status | Reason |
|-------------|--------|--------|
| Virtual functions | Not used | Indirect call prevents inlining |
| RTTI (`typeid`) | Not used | Runtime overhead for inspection |
| Exceptions | Not used | Stack unwinding costs |
| Constructors/Destructors | Not used | Direct assignment instead |
| Smart pointers | Not used | Block memory replaces ownership tracking |

---

## What We Don't Have (And Why)

| Missing Feature | Performance Impact | Alternative in Meta-C |
|----------------|-------------------|----------------------|
| Exceptions | Runtime overhead + larger binary + slower code paths | `error()` panic for unrecoverable |
| RTTI | `typeid()` is expensive, `dynamic_cast` even worse | Structs know their type at compile time |
| Virtual dispatch | Indirect call prevents inlining, breaks branch prediction | Direct function calls always |
| Garbage Collector | "Stop the world" pauses, cache pollution | `block.reset()` is instant and deterministic |
| Individual free | `free()` is O(1) but causes fragmentation | `block.reset()` for bulk deallocation |
| Stack for user data | Stack is limited, can't hold large structs | Everything goes to blocks |
| Reference counting | Atomic ops on every assignment | Block lifetime management |

---

## SIMD Alignment

The bump allocator supports configurable alignment for SIMD data:

```c
// 16-byte alignment for SSE
void* ptr = block_alloc_aligned(ctx, size, 16);

// 32-byte alignment for AVX
void* ptr = block_alloc_aligned(ctx, size, 32);
```

Future optimization: autovectorization hints via `__attribute__((aligned(32)))` on float arrays.

---

## Memory Overhead Comparison

| Feature | malloc | Meta-C Block |
|---------|--------|--------------|
| Per-allocation header | 8-16 bytes | **0 bytes** |
| Fragmentation | 10-30% | **0%** |
| Free list overhead | Yes | **None** |
| Lock contention | Yes (thread-safe) | **None** (single-thread default) |
| Total memory used per 1GB | 1.1-1.3 GB | **Exactly 1 GB** |

---

## Compilation Performance

| Metric | Expected |
|--------|----------|
| 100 lines .mc → C | <10ms |
| 1000 lines .mc → C | <50ms |
| 10000 lines .mc → C | <500ms |
| Memory used during compilation | ~10-50 MB |

These are fast because:
- No IR generation or optimization
- No type inference (all types are explicit)
- No LLVM backend
- Direct C text output

---

## Future Optimizations Planned

- [x] **Bump allocator with configurable alignment** — done
- [x] **Flat structs (no vtable)** — done
- [ ] **SIMD alignment hints** — 16/32 byte alignment for float arrays
- [ ] **Thread-local blocks** — each thread with its own block, no lock
- [ ] **Constant folding** — pre-compute constant expressions in codegen
- [ ] **Inline hints** — `__attribute__((always_inline))` in generated code
- [ ] **Pool allocator** — for small objects (like String)
- [ ] **Pauseless hot reload** — double-buffering for block memory
- [ ] **Profile-guided optimization (PGO)** — in release build
- [ ] **Dead struct field elimination** — remove unused fields
- [ ] **Loop unrolling hints** — for hot loops

---

## Tips for Maximum Performance

### 1. Choose Block Sizes Wisely

```meta-c
// Too small → overflow panics
block temp = 1KB    // dangerous

// Too large → memory waste
block temp = 1GB    // wasteful

// Just right → use visualizer to measure
block temp = 8MB    // fit your workload
```

### 2. Reset Blocks at Natural Boundaries

```meta-c
fn game_loop() {
    temp.reset()       // every frame
    level.reset()      // on level transition
    global.reset()     // on shutdown
}
```

### 3. Use Release Builds for Production

```bash
# With tracking (for debugging)
gcc -O3 -DMETA_C_TRACK_BLOCKS -o program *.c -ldl

# Without tracking (max performance)
gcc -O3 -o program *.c -ldl
```

When `META_C_TRACK_BLOCKS` is not defined, the block registry operations become no-op macros that the compiler optimizes away entirely.

### 4. Profile-Guided Code Organization

- Group frequently accessed data in the same block (better cache locality)
- Use `@` annotations sparingly (block scope is clearer)
- Reset large blocks less frequently (reset time is O(1) regardless of size)

### 5. Let gcc Optimize

Meta-C generates simple C code that gcc optimizes well:

```c
// Generated C is straightforward — gcc loves this
int64_t Player_take_damage(Player* this, int64_t dmg) {
    this->hp -= dmg;
    return this->hp;
}
```

Don't try to outsmart the compiler. Write clear Meta-C code and let `-O3` do its magic.

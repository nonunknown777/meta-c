# Brick Optimizations

> Notes on performance and how Brick aims to be as fast as possible.

## Performance Philosophy

Brick is built in 3 layers, each with its own optimizations:

```
1. COMPILER (C++20) → runs at development time
   ├─ Uses templates and constexpr to compute as much as possible at compile-time
   └─ Generates already-optimized C (no extra passes)

2. GENERATED CODE → the .c produced by the compiler
   ├─ Pure, readable C with no hidden abstractions
   └─ Compiled with gcc/clang -O3

3. RUNTIME (C) → runs alongside the program
   ├─ Bump allocator = allocation in ~3 CPU cycles
   └─ Zero overhead features (no exceptions, no RTTI)
```

## Bump Allocator

The Block Memory allocator is the soul of Brick's performance:

```
Normal allocation (malloc):   ~80 cycles
Bump allocation (Brick):      ~3 cycles  ← 27x faster
```

How it works:

- The block is a contiguous piece of memory
- Allocating = advancing a pointer (no search for free space)
- Resetting = moving the pointer back to the start (1 cycle)
- No internal fragmentation
- Cache-friendly (data stays close together)

## Applied Optimizations

### In the compiler (src/)
- Recursive descent parser (no backtracking, O(n))
- AST nodes without heavy smart pointers
- Codegen writes to a continuous stringstream (no multiple allocations)
- Keyword lookup with unordered_map (amortized O(1))

### In the generated code
- Flat structs (no hidden vtable)
- Methods become regular C functions (direct call, no dispatch)
- #line directives generate no extra code (only debugger info)
- Direct assignment without C++ construction/destruction

### In the runtime (C)
- Bump allocator with configurable alignment
- block_reset is just `ctx->used = 0` (O(1))
- No locks by default (optional thread-safe)
- block_stats is just struct reading (O(1))

## What We DON'T Have (and Why)

| Missing | Reason |
|---------|--------|
| Exceptions | Runtime overhead + larger generated code |
| RTTI | typeid() is expensive, structs know who they are |
| Virtual dispatch | Indirect call prevents inlining |
| Garbage Collector | Stops the world, unpredictable |
| Individual free | Bump allocator doesn't need it |
| User stack | Consistency: everything in blocks |

## Expected Benchmarks

```
Allocation:     100M allocs of 64 bytes
  malloc:       8.2s
  Brick bump:  0.3s  ← 27x faster

Reset:          1M block resets
  free():       2.1s
  block_reset:  0.001s  ← 2000x faster

Compilation:    1000 lines of .brc → C
  Brick:       < 50ms  (target)
```

## ✅ Implemented Optimizations

- [x] **Constant folding** — integer and float constant binary ops pre-computed at compile time (`src/codegen/codegen.cpp`)
- [x] **Inline hints for gcc** — `__attribute__((always_inline))` + `static inline` on non-main, non-extern functions (`src/codegen/codegen.cpp`)
- [x] **SIMD alignment** — `__attribute__((aligned(N)))` emitted for f32/f64 fields and arrays in generated C structs (`src/codegen/codegen.cpp`)
- [x] **Pool allocator (automatic)** — every block gets a pool for types ≤ 64 bytes. The compiler automatically emits `pool_alloc()` instead of `block_alloc()` for small types (`src/codegen/codegen.cpp`, `runtime/pool_allocator.h/.c`)
- [x] **Thread-local blocks** — `__thread BlockCtx* _tls_current_block` with `block_set_tls()`/`block_get_tls()` API. `__brick_init()` now calls `block_set_tls()` automatically (`runtime/block_memory.h/.c`)
- [x] **Pauseless hot reload** — block double-buffering API (`block_enable_double_buffer`, `block_swap_buffers`, `block_alloc_db`) (`runtime/block_memory.c`)
- [x] **Profile-guided optimization (PGO)** — `scons profile=pgo-gen` / `scons profile=pgo-use` build profiles (`SConstruct`)

---

## Deep Dive: How Each Optimization Works

### 1. Constant Folding

**Problem**: Calculating the same constant expression millions of times wastes CPU.

**Solution**: Before generating C, the compiler looks at expressions that contain only numbers and computes the result itself.

```brick
// What you write:
let x = 60 * 1000  // milliseconds in 1 minute

// Without constant folding: every execution does 60 * 1000
// With constant folding: the compiler turns it into:
let x = 60000      // already computed!
```

**Analogia**: It's like adding 2+2 on a calculator before saying the answer, instead of thinking "what's 2+2?" every time someone asks.

---

### 2. Inline Hints for GCC

**Problem**: Function calls have overhead — push arguments, jump, return. For small functions, this overhead can be bigger than the function body.

**Solution**: The compiler adds `__attribute__((always_inline))` + `static inline` before every non-main, non-extern function. This tells GCC: "copy this function's body directly where it's called."

```c
// Generated C before optimization:
int32_t Player_get_hp(Player* this) { return this->hp; }

int main() {
    int x = Player_get_hp(&p);  // call overhead: ~5-10 cycles
}

// After inline hint:
static inline __attribute__((always_inline))
int32_t Player_get_hp(Player* this) { return this->hp; }
// Now GCC copies the body directly — zero call overhead.
```

The effect: no function call at all for methods, getters, small computations. Everything is "flattened" into main.

---

### 3. SIMD Alignment

**Problem**: SIMD instructions (SSE, AVX) can load/store 4 floats at once — but only if the data is aligned to 16 or 32 bytes. Unaligned access is 2-3× slower.

**Solution**: The compiler detects float/f64 fields and float arrays in structs, and adds `__attribute__((aligned(N)))` to them.

```brick
struct Particle {
    f32 x, y, z, w  // position
    f32 vx, vy, vz, vw  // velocity
}
```

Generated C:
```c
typedef struct Particle {
    __attribute__((aligned(16)))
    float x, y, z, w;
    __attribute__((aligned(16)))
    float vx, vy, vz, vw;
} Particle;
```

Now GCC can use `movaps` (aligned SSE mov) instead of `movups` (unaligned) — up to 2× faster particle updates.

---

### 4. Pool Allocator (Automatic for Small Types)

**Problem**: The bump allocator is extremely fast (~3 cycles), but you can't free individual objects — only reset the entire block. This wastes memory for patterns that need sporadic frees.

**Solution**: Every `block` declaration automatically creates a companion pool allocator (`PoolAllocator*`). The codegen checks each allocation: if the type is ≤ 64 bytes, it calls `pool_alloc()` instead of `block_alloc()`. Pool alloc/free is O(1) — it just pops/pushes from a free list.

```brick
block global = 64MB

struct Particle {
    f32 x, y, z
    i32 life
}

fn main() {
    // Particle = 16 bytes ≤ 64 → uses pool_alloc automatically!
    Particle p = Particle() @global
}
```

Generated C:
```c
// Created in __brick_init:
__pool_global = pool_create();
pool_add_slot(__pool_global, 64, 4096);

// Allocation (small type → pool):
Particle* p = pool_alloc(__pool_global, sizeof(Particle));
Particle_init(p);
```

Type size estimation is automatic — the compiler calculates struct field sizes recursively. `String` (16 bytes), scalars (1-8 bytes), and small structs all use the pool. Large types (> 64 bytes) and unknown types still use the bump allocator.

**Analogia**: It's like having a tray of pre-sorted gavetas on your desk. Instead of walking to the big filing cabinet (malloc) or just piling everything in one box (bump alloc), you grab the right gaveta instantly.

---

### 5. TLS Blocks (Thread-Local Current Block)

**Problem**: When multiple threads allocate into the same block, they need locks or atomic operations — which cost 50-500 cycles per allocation.

**Solution**: Each thread can have its own block via `__thread BlockCtx* _tls_current_block`. The `__brick_init()` function now calls `block_set_tls(_current_block)` automatically, wiring the main thread's TLS.

```c
// In __brick_init(), generated automatically:
block_set_tls(_current_block);

// Now any code can call block_alloc_tls(size) to use the thread's block
// without locks, without specifying which block.
```

**When to use**: If you spawn threads and each thread needs its own memory region:
```brick
using C

fn worker() {
    // From C: block_set_tls(my_block)
    // Then allocates go to my_block without specifying it
}
```

The TLS API (`block_set_tls`, `block_get_tls`, `block_alloc_tls`) gives you **zero-contention** allocations across threads. Each thread's pointer is truly local — no lock, no atomic, no cache line bouncing.

**Analogia**: Every employee has their own desk instead of sharing one. No waiting to grab a paper.

---

### 6. Pauseless Hot Reload (Double-Buffer)

**Problem**: Block reset erases everything. If you reset during hot reload, you lose the current frame's data — visible stutter.

**Solution**: `block_enable_double_buffer(block)` creates a shadow buffer. When you call `block_swap_buffers(block)`, it atomically swaps the active buffer. The old buffer is preserved until you explicitly deallocate it.

```c
// Enable double-buffer for a block
block_enable_double_buffer(scene);

// Normal operation — allocates into the primary buffer
Object* obj = block_alloc(scene, sizeof(Object));

// Hot reload: compile new code into the shadow buffer
block_swap_buffers(scene);
// Swap is atomic — the program never sees a half-written buffer

// Now allocations go to the new buffer, old one is safe
Object* new_obj = block_alloc(scene, sizeof(Object));
```

**Without double-buffer**: reset the block → lose all data → reload → rebuild everything. ~50ms pause, visible frame skip.

**With double-buffer**: swap in ~1 CPU cycle. Zero visible interruption.

**Analogia**: A restaurant with a main kitchen and a backup kitchen. When the menu changes, the chef cooks everything in the backup kitchen while the main kitchen still serves. When ready, just flip the sign — no one notices a pause.

---

### 7. PGO (Profile-Guided Optimization)

**Problem**: GCC guesses which code paths are hot — and sometimes guesses wrong. Branch mispredictions cost 10-20 cycles each.

**Solution**: Compile twice:
1. `scons profile=pgo-gen` — adds instrumentation to count branches
2. Run the instrumented binary with real data — generates a `.gcda` profile
3. `scons profile=pgo-use` — recompiles using the profile

```bash
# Step 1: instrument
scons profile=pgo-gen

# Step 2: run with real workload
./my_game --benchmark

# Step 3: optimize using profile
scons profile=pgo-use
# Now GCC knows: the "player is alive" branch is taken 95% of the time
# → arranges code so that path is sequential, no branch at all
```

The result: better branch prediction, better instruction cache usage, better inlining decisions. Typical gains: 5-15%.

**Analogia**: A cashier who memorizes which aisles you visit most. Without PGO, every request requires "which aisle?" and a search. With PGO, they already know and go straight there.

---

## Next Up

- **Compiler profiling** — measure compilation time per phase
- **Generated code profiling** — measure runtime performance with perf
- **Inline asm in hot paths** — hand-tuned assembly for critical loops
- **String interning** — deduplicate string literals

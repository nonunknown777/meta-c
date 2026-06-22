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

## Planned Future Optimizations

- [ ] SIMD alignment (16/32 bytes) for float arrays
- [ ] Thread-local blocks (each thread with its own block, no lock)
- [ ] Codegen with constant folding (pre-compute constant expressions)
- [ ] Inline hints for gcc (`__attribute__((always_inline))`)
- [ ] Pool allocator for small objects (like String)
- [ ] Pauseless hot reload (block double-buffering)
- [ ] Profile-guided optimization (PGO) in release build

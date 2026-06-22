# Memory Blocks

The block memory system is the heart of Brick's performance philosophy. Instead of traditional malloc/free or garbage collection, Brick uses **bump allocation** in user-declared memory blocks.

---

## Why Blocks?

Traditional memory management has three approaches, each with drawbacks:

| Approach | Problem |
|----------|---------|
| `malloc`/`free` | Fragmentation, O(n) allocation, cache misses |
| Garbage Collection | "Stop the world" pauses, unpredictable performance |
| Manual (C++) | `new`/`delete` everywhere, leak-prone, complex |

Brick's approach: **declare blocks, allocate into them, reset when done**.

```
Allocation: +++  (3 CPU cycles — just advance a pointer)
                   No search, no fragmentation, no locks
Reset:       +    (1 CPU cycle — just set used = 0)
                   No individual frees, no sweeping
```

---

## Block Declaration

Blocks are declared at the file level (outside any function):

```brick
block global = 256MB    // Units: KB, MB, GB
block game   = 64MB
block temp   = 8KB
block assets = 128MB
block data   = 1GB
```

**Rules:**

- `global` **must** be declared in the main file — it is the default block
- Block names must be unique
- Supported size units: `KB` (kilobytes), `MB` (megabytes), `GB` (gigabytes)
- Block memory is pre-allocated at program start (`__brick_init()`)

---

## Allocation

### 1. Default Allocation (to `global`)

Variables without any block annotation go to the `global` block:

```brick
int x = 5           // allocated in 'global'
String s = "hello"  // also in 'global'
```

### 2. Block Scope

Use `block name { }` to temporarily change the default block:

```brick
block game {
    Player p = Player(100, "Felipe")    // allocated in 'game'
    Enemy e = Enemy(50)                 // also in 'game'

    // Nested: you can have another block scope inside
    block temp {
        String log_msg = "debug"        // allocated in 'temp'
    }
}
```

The compiler generates code that saves and restores the current block context:

```c
// Generated C equivalent:
{
    BlockCtx* _old1 = _current_block;
    _current_block = game;
    // ... allocations go to 'game' ...
    _current_block = _old1;
}
```

### 3. Inline Annotation (`@`)

Allocate a specific variable in a specific block:

```brick
float f = 2.0 @temp             // f lives in 'temp'
int[] arr = int[10] @game       // arr lives in 'game'
Player p = Player(100, "Felipe") @game
```

The annotation goes **after** the value expression, before the semicolon.

### 4. Block-Allocated Constructors

When you call a constructor with `@`:

```brick
Player p = Player(100, "Felipe", 30) @game
```

The compiler generates C code that:
1. Allocates raw memory in the block: `block_alloc(game, sizeof(Player))`
2. Calls the constructor with the pointer: `Player_Player(p, 100, "Felipe", 30)`

This is equivalent to:
```c
Player* p = block_alloc(game, sizeof(Player));
Player_Player(p, 100, "Felipe", 30);
```

---

## Reset

The only way to free memory in Brick is to reset an entire block:

```brick
game.reset()       // O(1) — just sets used = 0
```

```c
// Generated C:
block_reset(game);  // ctx->used = 0;
```

**What reset does:**
- Sets `ctx->used` back to 0 (the bump pointer goes to the start)
- Does **not** zero the memory (that would be wasteful)
- Does **not** decrement the allocation counter (it tracks total allocations across resets)
- Auto-exports to shared memory (for the visualizer)

**What reset does NOT do:**
- Call destructors (there are none in Brick)
- Free individual objects
- Check for dangling pointers

---

## Cross-Block References

Pointers can refer across blocks:

```brick
block global = 256MB
block temp = 8MB

struct Reference {
    String data_pointer    // points to data in another block
}

fn main() {
    String big_data = "this is a very long string" @global

    block temp {
        Reference r = Reference(big_data)  // r lives in temp,
                                           // but r.data_pointer points to global
    }

    // This is legal — r is in temp but references data in global
    // But if you reset global, r.data_pointer becomes dangling!
}
```

**Warning:** Resetting a block does not invalidate pointers from other blocks. The programmer is responsible for ensuring no dangling references exist.

---

## Block Overflow

If a block runs out of space, the program panics:

```
Brick runtime error: block overflow: out of memory in block
```

The panic is intentional — it's better than silent corruption. Choose block sizes carefully based on your usage patterns.

---

## Best Practices

### 1. Match Block Size to Lifetime

```brick
block global = 256MB    // program lifetime — reset only at exit
block level  = 64MB     // per-level data — reset when changing levels
block temp   = 8MB      // per-frame data — reset every frame
block frame  = 1MB      // very short-lived
```

### 2. Reset at Natural Boundaries

```brick
fn game_loop() {
    while running {
        temp.reset()       // clear temp data every frame
        level.reset()      // clear level when transitioning

        // ... game logic ...
    }
    global.reset()         // clean up at exit
}
```

### 3. Use `temp` for Short-Lived Data

```brick
fn process() {
    String debug_msg = "processing..." @temp  // temporary string
    int[1000] buffer = int[1000] @temp        // temporary buffer
    // ... use buffer ...
    temp.reset()                               // discard all temp data
}
```

### 4. Profile Before Sizing

Start generous and use the visualizer or `block_stats()` to see actual usage:

```
global  256MB  ████████░░░░░░  42%
game     64MB  ██████████████  89%  ⚠
temp      8MB  ██░░░░░░░░░░░░  15%
```

The `⚠` warning appears at 80%+ usage — resize blocks accordingly.

### 5. No Individual Free

If you're used to `free()` or `delete`, adjust your thinking:
- Batch allocations by lifetime
- Group objects that die together into the same block
- Reset the whole block when done

### 6. Memory Leak Prevention

Brick's block system makes leaks **impossible**:
- Every allocation goes to a block
- When the block resets, everything is reclaimed
- No need to track individual allocations
- No destructors, no reference counting

---

## Runtime API Reference

The block memory system is implemented in `runtime/block_memory.c`. The API is also available for direct use in generated C code:

```c
// Create blocks
BlockCtx* block_create(size_t megabytes);
BlockCtx* block_create_bytes(size_t bytes);

// Allocate
void* block_alloc(BlockCtx* ctx, size_t size);
void* block_alloc_aligned(BlockCtx* ctx, size_t size, size_t alignment);

// Reset / Destroy
void block_reset(BlockCtx* ctx);     // O(1) — just used = 0
void block_destroy(BlockCtx* ctx);   // free memory

// Stats
BlockStats block_stats(BlockCtx* ctx);

// Hot reload synchronization
void block_freeze(void);  // Spin-wait until thawed
void block_thaw(void);
```

### Internal Structure

```c
typedef struct BlockCtx {
    uint8_t*  data;             // pointer to the block's memory
    size_t    capacity;         // total size in bytes
    size_t    used;             // current bump offset
    size_t    peak_used;        // highest used value since creation
    size_t    allocation_count; // total allocations (never reset)
} BlockCtx;
```

### Block Alignment

Default alignment is **8 bytes**. Use `block_alloc_aligned()` for SIMD-ready data:

```c
// 16-byte alignment for SSE
float* simd_data = block_alloc_aligned(ctx, 64 * sizeof(float), 16);

// 32-byte alignment for AVX
double* avx_data = block_alloc_aligned(ctx, 32 * sizeof(double), 32);
```

---

## Shared Memory Export

For the TUI visualizer and VS Code extension, block information is auto-exported to `/tmp/brick-mem-<pid>.bin` every 16 allocations. This file contains:

```c
typedef struct {
    uint32_t magic;       // 0x4D455441 ("META")
    uint32_t version;
    int32_t  pid;
    uint32_t block_count;
    uint64_t timestamp_us;
} BrickShmHeader;

typedef struct {
    char     name[32];
    size_t   capacity;
    size_t   used;
    size_t   peak_used;
    size_t   allocation_count;
} BlockInfo;
```

Export is only enabled when compiled with `-DBRICK_TRACK_BLOCKS`.

---

## Performance Characteristics

| Operation | Cycles | Compared to |
|-----------|:------:|-------------|
| `block_alloc` (bump) | ~3 | malloc: ~80 cycles (27x faster) |
| `block_reset` | ~1 | free() loop: ~2000x faster |
| `block_stats` | ~1 | struct read |
| Memory density | 0% fragmentation | malloc: ~10-30% overhead |
| Cache behavior | Excellent (contiguous) | malloc: scattered |

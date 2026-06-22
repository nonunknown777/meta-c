# Task: Runtime (Brick)

## Função
## Role

Você é o especialista em RUNTIME do Brick.
Responsabilidade: implementar o sistema de blocos de memória em C.
You are the RUNTIME specialist for Brick.
Responsibility: implement the block memory system in C.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md
2. ANTES DE SAIR: atualize estado
3. Código em: /mnt/Novo_volume/brick/runtime/
4. Testes em: /mnt/Novo_volume/brick/tests/test_runtime.c
5. HEADERS DEVEM TER extern "C" pra compatibilidade C++

1. ON START: read STATE.md, NEXT.md and shared-context.md
2. BEFORE LEAVING: update state
3. Code in: /mnt/Novo_volume/brick/runtime/
4. Tests in: /mnt/Novo_volume/brick/tests/test_runtime.c
5. HEADERS MUST HAVE extern "C" for C++ compatibility

## API Pública (block_memory.h)
## Public API (block_memory.h)

```c
#ifndef BRICK_BLOCK_MEMORY_H
#define BRICK_BLOCK_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BlockCtx BlockCtx;

typedef struct {
    size_t total_size;
    size_t used_size;
    size_t free_size;
    size_t peak_used;
    size_t allocation_count;
    float  fragmentation_percent;
} BlockStats;

BlockCtx*   block_create(size_t megabytes);
void*       block_alloc(BlockCtx* ctx, size_t size);
void        block_reset(BlockCtx* ctx);
void        block_destroy(BlockCtx* ctx);
BlockStats  block_stats(BlockCtx* ctx);
size_t      block_alignment(void);

#ifdef __cplusplus
}
#endif

#endif
```

## Flag de Compilação: BRICK_TRACK_BLOCKS
## Compilation Flag: BRICK_TRACK_BLOCKS

O runtime pode opcionalmente manter um **registro global de blocos** para tools
externas (visualizador, debugger). Esse registro é ativado pela flag de
compilação `-DBRICK_TRACK_BLOCKS`.
The runtime can optionally maintain a **global block registry** for external
tools (visualizer, debugger). This registry is activated by the
`-DBRICK_TRACK_BLOCKS` compilation flag.

**Quando ativado:**
**When activated:**

- `block_register(ctx, name)` — adiciona bloco ao registro
- `block_unregister(ctx)` — remove do registro
- `block_find(name)` — busca por nome
- `block_snapshot(out, max)` — snapshot thread-safe
- `block_shm_export()` — exporta pra `/tmp/brick-mem-<pid>.bin`
- Consumo: ~3KB BSS (64 entradas × 40 bytes + mutex)

- `block_register(ctx, name)` — adds block to registry
- `block_unregister(ctx)` — removes from registry
- `block_find(name)` — search by name
- `block_snapshot(out, max)` — thread-safe snapshot
- `block_shm_export()` — exports to `/tmp/brick-mem-<pid>.bin`
- Overhead: ~3KB BSS (64 entries × 40 bytes + mutex)

**Quando desativado (default):**
**When disabled (default):**

- Macros no-op → **zero custo, zero memória, zero código**
- O compilador elimina qualquer chamada a essas funções
- No-op macros → **zero cost, zero memory, zero code**
- The compiler eliminates any calls to these functions

**Quem ativa:**
**Who activates it:**

- O `SConstruct` adiciona `-DBRICK_TRACK_BLOCKS` automaticamente nos
  CFLAGS/CXXFLAGS quando `visualizer=yes` (default).
- The `SConstruct` adds `-DBRICK_TRACK_BLOCKS` automatically to
  CFLAGS/CXXFLAGS when `visualizer=yes` (default).

**IMPORTANTE:**
**IMPORTANT:**

- `block_destroy()` NÃO chama `block_unregister()` — o usuário deve
  desregistrar antes de destruir.
- `BRICK_BLOCK_NAME_MAX = 32`, `BRICK_MAX_BLOCKS = 64`
- Thread-safe via `pthread_mutex_t` no registry (não afeta o bump allocator)

- `block_destroy()` does NOT call `block_unregister()` — the user must
  unregister before destroying.
- `BRICK_BLOCK_NAME_MAX = 32`, `BRICK_MAX_BLOCKS = 64`
- Thread-safe via `pthread_mutex_t` in the registry (does not affect the bump allocator)

## Implementação
## Implementation

- Bump allocator puro (performático, sem free individual)
- Alinhamento a 8 bytes (ou 16 pra SIMD futuro)
- Bloco é um mmap ou malloc gigante contíguo
- block_reset só reseta o ponteiro do bump (O(1))
- block_stats útil pro visualizador
- Thread-safe: opcional (mutex ou não, decisão sua)
- Segurança: detectar overflow no block_alloc e chamar error()

- Pure bump allocator (high performance, no individual free)
- 8-byte alignment (or 16 for future SIMD)
- Block is a contiguous mmap or giant malloc
- block_reset only resets the bump pointer (O(1))
- block_stats useful for the visualizer
- Thread-safe: optional (mutex or not, your decision)
- Safety: detect overflow in block_alloc and call error()

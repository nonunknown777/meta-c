#ifndef BRICK_BLOCK_MEMORY_H
#define BRICK_BLOCK_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BlockCtx {
    uint8_t*  data;
    size_t    capacity;
    size_t    used;
    size_t    peak_used;
    size_t    allocation_count;
} BlockCtx;

typedef struct {
    size_t total_size;
    size_t used_size;
    size_t free_size;
    size_t peak_used;
    size_t allocation_count;
    float  fragmentation_percent;
} BlockStats;

// ─── Registry API (optional — requires -DBRICK_TRACK_BLOCKS) ─
// ─── API do Registro (opcional — requer -DBRICK_TRACK_BLOCKS) ─
#ifdef BRICK_TRACK_BLOCKS

#define BRICK_BLOCK_NAME_MAX 32
#define BRICK_MAX_BLOCKS 64

typedef struct {
    char     name[BRICK_BLOCK_NAME_MAX];
    size_t   capacity;
    size_t   used;
    size_t   peak_used;
    size_t   allocation_count;
} BlockInfo;

#define BRICK_SHM_MAGIC 0x4D455441  // "META"

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    int32_t  pid;
    uint32_t block_count;
    uint64_t timestamp_us;
} BrickShmHeader;

void     block_register(BlockCtx* ctx, const char* name);
void     block_unregister(BlockCtx* ctx);
BlockCtx* block_find(const char* name);
size_t   block_snapshot(BlockInfo* out, size_t max_count);
int      block_shm_export(void);

#else
// No-op stubs — compiler optimizes these away entirely
// Stubs no-op — compilador otimiza esses totalmente
#define BRICK_MAX_BLOCKS 1
#define BRICK_BLOCK_NAME_MAX 1
#define block_register(ctx, name)     ((void)(ctx), (void)(name))
#define block_unregister(ctx)         ((void)(ctx))
#define block_find(name)              ((void)(name), (BlockCtx*)NULL)
#define block_snapshot(out, max)      ((void)(out), (void)(max), (size_t)0)
#define block_shm_export()            ((void)0)
#endif

// ─── Block API ───────────────────────────────────────────────
// ─── API de Blocos ────────────────────────────────────────────

// Create a new memory block of N megabytes
// Cria um novo bloco de memoria de N megabytes
BlockCtx* block_create(size_t megabytes);

// Create a block with custom byte size
// Cria um bloco com tamanho personalizado em bytes
BlockCtx* block_create_bytes(size_t bytes);

// Allocate memory from a block (bump allocator)
// Aloca memoria de um bloco (bump allocator)
void* block_alloc(BlockCtx* ctx, size_t size);

// Allocate with alignment
// Aloca com alinhamento
void* block_alloc_aligned(BlockCtx* ctx, size_t size, size_t alignment);

// Reset the block (O(1) — just resets bump pointer)
// Reseta o bloco (O(1) — apenas reseta o ponteiro bump)
void block_reset(BlockCtx* ctx);

// Destroy the block and free its memory
// Destroi o bloco e libera sua memoria
void block_destroy(BlockCtx* ctx);

// Get block statistics
// Obtem estatisticas do bloco
BlockStats block_stats(BlockCtx* ctx);

// Get the default alignment
// Obtem o alinhamento padrao
size_t block_alignment(void);

// Freeze all block allocations (spin-wait until thawed)
// Congela todas as alocacoes de bloco (espera ocupada ate descongelar)
// Used by hot reload to ensure no allocations during code swap
// Usado pelo hot reload para garantir nenhuma alocacao durante troca de codigo
void block_freeze(void);

// Thaw block allocations after hot reload swap
// Descongela alocacoes de bloco apos troca de hot reload
void block_thaw(void);

// ─── Thread-Local Blocks ─────────────────────────────────────
// Blocos com thread-local
// ──────────────────────────────────────────────────────────────

// Declare a thread-local current block pointer
// Declara um ponteiro de bloco atual thread-local
// Each thread can have its own current block, avoiding locks
// Cada thread pode ter seu proprio bloco atual, evitando locks
extern __thread BlockCtx* _tls_current_block;

// Set the current thread-local block
// Define o bloco atual thread-local
void block_set_tls(BlockCtx* ctx);

// Get the current thread-local block
// Obtem o bloco atual thread-local
BlockCtx* block_get_tls(void);

// ─── Pauseless Hot Reload: Block Double-Buffering ───────────
// Hot Reload sem pausa: Double-buffer de blocos
// ──────────────────────────────────────────────────────────────

// Enable double-buffer mode on a block
// Ativa modo double-buffer em um bloco
// Maintains two copies of block data; on hot reload, swaps atomically
// Mantem duas copias dos dados do bloco; no hot reload, troca atomicamente
int  block_enable_double_buffer(BlockCtx* ctx);

// Swap the active and shadow buffers atomically (called by hot reload)
// Troca os buffers ativo e sombra atomicamente (chamado pelo hot reload)
void block_swap_buffers(BlockCtx* ctx);

// Allocate from the active buffer (same as block_alloc but aware of double-buffer)
// Aloca do buffer ativo (mesmo que block_alloc mas ciente de double-buffer)
void* block_alloc_db(BlockCtx* ctx, size_t size);

// Check if a block has double-buffer enabled (for debugger)
// Verifica se um bloco tem double-buffer ativado (para o debugger)
int block_has_double_buffer(BlockCtx* ctx);

#ifdef __cplusplus
}
#endif

#endif // BRICK_BLOCK_MEMORY_H
     // BRICK_BLOCK_MEMORY_H

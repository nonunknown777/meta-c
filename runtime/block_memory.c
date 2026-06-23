#define _GNU_SOURCE
#include "block_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/mman.h>

#define DEFAULT_ALIGNMENT 8

// Compute optimal alignment for a given allocation size
// Computa alinhamento otimo para um dado tamanho de alocacao
// size=1→1, 2→2, 3→2, 4→4, 5→4, 6→4, 7→4, 8→8, 9→8, ...
// Eliminates padding waste for small types (u8, u16, u32, f32)
// Elimina desperdicio de padding para tipos pequenos (u8, u16, u32, f32)
static inline size_t optimal_alignment(size_t size) {
    return size >= 8 ? (size_t)8
         : size >= 4 ? (size_t)4
         : size >= 2 ? (size_t)2
         :            (size_t)1;
}

static atomic_int block_frozen_flag = 0;

// Thread-local current block
// Bloco atual thread-local
__thread BlockCtx* _tls_current_block = NULL;

// Double-buffer support
// Suporte a double-buffer
typedef struct {
    uint8_t* shadow_data;
    size_t   shadow_capacity;
    size_t   shadow_used;
    int      active_buffer; // 0 = primary, 1 = shadow
                            // 0 = primario, 1 = sombra
} BlockDoubleBuffer;

#define DB_MAGIC 0xDBDBDBDB
typedef struct {
    uint32_t          magic;
    BlockDoubleBuffer db;
} BlockExtension;

// ─── Global Block Registry ───────────────────────────────────
// ─── Registro Global de Blocos ────────────────────────────────
#ifdef BRICK_TRACK_BLOCKS

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#define REGISTRY_MAX BRICK_MAX_BLOCKS

typedef struct {
    char      name[BRICK_BLOCK_NAME_MAX];
    BlockCtx* ctx;
    int       active;
} RegistryEntry;

static RegistryEntry     registry[REGISTRY_MAX];
static int               registry_initialized = 0;
static pthread_mutex_t   registry_mutex = PTHREAD_MUTEX_INITIALIZER;

static void registry_init(void) {
    if (!registry_initialized) {
        memset(registry, 0, sizeof(registry));
        registry_initialized = 1;
    }
}

void block_register(BlockCtx* ctx, const char* name) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    int slot = -1;
    for (int i = 0; i < REGISTRY_MAX; i++) {
        if (!registry[i].active) { slot = i; break; }
    }

    if (slot >= 0) {
        strncpy(registry[slot].name, name, BRICK_BLOCK_NAME_MAX - 1);
        registry[slot].name[BRICK_BLOCK_NAME_MAX - 1] = '\0';
        registry[slot].ctx = ctx;
        registry[slot].active = 1;
    }

    pthread_mutex_unlock(&registry_mutex);
}

void block_unregister(BlockCtx* ctx) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    for (int i = 0; i < REGISTRY_MAX; i++) {
        if (registry[i].active && registry[i].ctx == ctx) {
            registry[i].active = 0;
            break;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
}

BlockCtx* block_find(const char* name) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    BlockCtx* found = NULL;
    for (int i = 0; i < REGISTRY_MAX; i++) {
        if (registry[i].active && strcmp(registry[i].name, name) == 0) {
            found = registry[i].ctx;
            break;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
    return found;
}

size_t block_snapshot(BlockInfo* out, size_t max_count) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    size_t written = 0;
    for (int i = 0; i < REGISTRY_MAX && written < max_count; i++) {
        if (registry[i].active) {
            strncpy(out[written].name, registry[i].name, BRICK_BLOCK_NAME_MAX - 1);
            out[written].name[BRICK_BLOCK_NAME_MAX - 1] = '\0';
            out[written].capacity        = registry[i].ctx->capacity;
            out[written].used            = registry[i].ctx->used;
            out[written].peak_used       = registry[i].ctx->peak_used;
            out[written].allocation_count = registry[i].ctx->allocation_count;
            written++;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
    return written;
}

// ─── Shared Memory Export ────────────────────────────────────
// ─── Exportacao de Memoria Compartilhada ──────────────────────

static void shm_path(char* buf, size_t bufsize) {
    snprintf(buf, bufsize, "/tmp/brick-mem-%d.bin", (int)getpid());
}

int block_shm_export(void) {
    registry_init();

    char path[256];
    shm_path(path, sizeof(path));

    pthread_mutex_lock(&registry_mutex);

    int count = 0;
    for (int i = 0; i < REGISTRY_MAX; i++)
        if (registry[i].active) count++;

    size_t file_size = sizeof(BrickShmHeader) + (size_t)count * sizeof(BlockInfo);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { pthread_mutex_unlock(&registry_mutex); return -1; }

    ftruncate(fd, (off_t)file_size);

    BrickShmHeader header;
    header.magic   = BRICK_SHM_MAGIC;
    header.version = 1;
    header.pid     = (int32_t)getpid();
    header.block_count = (uint32_t)count;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    header.timestamp_us = (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;

    write(fd, &header, sizeof(header));

    int written = 0;
    for (int i = 0; i < REGISTRY_MAX && written < count; i++) {
        if (registry[i].active) {
            BlockInfo info;
            strncpy(info.name, registry[i].name, BRICK_BLOCK_NAME_MAX - 1);
            info.name[BRICK_BLOCK_NAME_MAX - 1] = '\0';
            info.capacity        = registry[i].ctx->capacity;
            info.used            = registry[i].ctx->used;
            info.peak_used       = registry[i].ctx->peak_used;
            info.allocation_count = registry[i].ctx->allocation_count;
            write(fd, &info, sizeof(info));
            written++;
        }
    }

    close(fd);
    pthread_mutex_unlock(&registry_mutex);
    return 0;
}

#endif // BRICK_TRACK_BLOCKS
     // BRICK_TRACK_BLOCKS

static void error(const char* msg) {
    fprintf(stderr, "Brick runtime error: %s\n", msg);
    exit(1);
}

BlockCtx* block_create(size_t megabytes) {
    return block_create_bytes(megabytes * 1024 * 1024);
}

BlockCtx* block_create_bytes(size_t bytes) {
    BlockCtx* ctx = (BlockCtx*)malloc(sizeof(BlockCtx));
    if (!ctx) error("out of memory");

    // Use mmap for large allocations (>= 64KB) — better for huge pages,
    // shared memory export, and cleaner deallocation
    // Usa mmap para alocacoes grandes (>= 64KB) — melhor para huge pages,
    // exportacao de memoria compartilhada e desalocacao mais limpa
    if (bytes >= 65536) {
        ctx->data = (uint8_t*)mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                                   -1, 0);
        if (ctx->data == MAP_FAILED) {
            free(ctx);
            error("out of memory (mmap failed)");
        }
    } else {
        ctx->data = (uint8_t*)malloc(bytes);
        if (!ctx->data) {
            free(ctx);
            error("out of memory");
        }
    }

    ctx->capacity = bytes;
    ctx->used = 0;
    ctx->peak_used = 0;
    ctx->allocation_count = 0;

    return ctx;
}

void* block_alloc(BlockCtx* ctx, size_t size) {
    // Use optimal alignment based on size to eliminate padding waste
    // Usa alinhamento otimo baseado no tamanho pra eliminar desperdicio de padding
    return block_alloc_aligned(ctx, size, optimal_alignment(size));
}

void* block_alloc_aligned(BlockCtx* ctx, size_t size, size_t alignment) {
    // Spin-wait if frozen (hot reload in progress)
    // Espera ocupada se congelado (hot reload em andamento)
    while (atomic_load_explicit(&block_frozen_flag, memory_order_acquire)) {
        // spin — will be very brief (nanoseconds)
        // giro — sera muito breve (nanossegundos)
        __asm__ volatile("pause");
    }

    // Align current position
    // Alinha posicao atual
    size_t current = ctx->used;
    size_t aligned = (current + alignment - 1) & ~(alignment - 1);

    // Check overflow
    // Verifica estouro
    if (aligned + size > ctx->capacity) {
        error("block overflow: out of memory in block");
    }

    ctx->used = aligned + size;
    ctx->allocation_count++;

    if (ctx->used > ctx->peak_used) {
        ctx->peak_used = ctx->used;
    }

    // Auto-export shm every 16 allocations (for visualizer attach mode)
    // Auto-exporta shm a cada 16 alocacoes (para modo attach do visualizer)
#ifdef BRICK_TRACK_BLOCKS
    static unsigned int _alloc_counter = 0;
    if ((++_alloc_counter & 0xF) == 0) {
        block_shm_export();
    }
#endif

    return ctx->data + aligned;
}

void block_reset(BlockCtx* ctx) {
    ctx->used = 0;
    // Note: allocation_count is NOT reset here,
    // Nota: allocation_count NAO e resetado aqui,
    // so we can track total allocations across resets
    // para que possamos rastrear alocacoes totais entre resets
#ifdef BRICK_TRACK_BLOCKS
    block_shm_export();
#endif
}

void block_destroy(BlockCtx* ctx) {
    if (ctx) {
        if (ctx->capacity >= 65536) {
            munmap(ctx->data, ctx->capacity);
        } else {
            free(ctx->data);
        }
        free(ctx);
    }
}

BlockStats block_stats(BlockCtx* ctx) {
    BlockStats stats;
    stats.total_size = ctx->capacity;
    stats.used_size = ctx->used;
    stats.free_size = ctx->capacity - ctx->used;
    stats.peak_used = ctx->peak_used;
    stats.allocation_count = ctx->allocation_count;

    // Fragmentation: 0% for bump allocator (no holes)
    // Fragmentacao: 0% para bump allocator (sem buracos)
    stats.fragmentation_percent = 0.0f;

    return stats;
}

size_t block_alignment(void) {
    return DEFAULT_ALIGNMENT;
}

void block_freeze(void) {
    atomic_store_explicit(&block_frozen_flag, 1, memory_order_release);
}

void block_thaw(void) {
    atomic_store_explicit(&block_frozen_flag, 0, memory_order_release);
}

// ─── Thread-Local Block API ──────────────────────────────────
// ─── API de Bloco Thread-Local ────────────────────────────────

void block_set_tls(BlockCtx* ctx) {
    _tls_current_block = ctx;
}

BlockCtx* block_get_tls(void) {
    return _tls_current_block;
}

// ─── Double-Buffer API (Pauseless Hot Reload) ───────────────
// ─── API de Double-Buffer (Hot Reload sem Pausa) ─────────────

static BlockCtx*    _db_ctx_table[64] = {NULL};
static BlockExtension* _db_ext_table[64] = {NULL};
static int          _db_table_count = 0;

static BlockExtension* find_db_ext(BlockCtx* ctx) {
    for (int i = 0; i < _db_table_count; i++) {
        if (_db_ctx_table[i] == ctx) return _db_ext_table[i];
    }
    return NULL;
}

int block_enable_double_buffer(BlockCtx* ctx) {
    BlockExtension* ext = (BlockExtension*)malloc(sizeof(BlockExtension));
    if (!ext) return -1;

    ext->magic = DB_MAGIC;
    ext->db.shadow_capacity = ctx->capacity;
    ext->db.shadow_used = 0;
    ext->db.active_buffer = 0;

    if (ctx->capacity >= 65536) {
        ext->db.shadow_data = (uint8_t*)mmap(NULL, ctx->capacity,
                                             PROT_READ | PROT_WRITE,
                                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                                             -1, 0);
        if (ext->db.shadow_data == MAP_FAILED) { free(ext); return -1; }
    } else {
        ext->db.shadow_data = (uint8_t*)malloc(ctx->capacity);
        if (!ext->db.shadow_data) { free(ext); return -1; }
    }

    if (_db_table_count >= 64) { free(ext); return -1; }

    _db_ctx_table[_db_table_count] = ctx;
    _db_ext_table[_db_table_count] = ext;
    _db_table_count++;
    return 0;
}

void block_swap_buffers(BlockCtx* ctx) {
    BlockExtension* ext = find_db_ext(ctx);
    if (!ext) return;

    // Atomically swap the active buffer
    // Troca atomicamente o buffer ativo
    int old_active = __sync_fetch_and_xor(&ext->db.active_buffer, 1);

    if (old_active == 0) {
        // Was using primary, now use shadow
        // Estava usando primario, agora usa sombra
        // Copy used count from active to shadow
        // Copia contagem usada do ativo para sombra
        ext->db.shadow_used = ctx->used;

        // Swap data pointers atomically
        // Troca ponteiros de dados atomicamente
        uint8_t* temp_data = ctx->data;
        ctx->data = ext->db.shadow_data;
        ext->db.shadow_data = temp_data;

        size_t temp_cap = ctx->capacity;
        ctx->capacity = ext->db.shadow_capacity;
        ext->db.shadow_capacity = temp_cap;
    }
}

void* block_alloc_db(BlockCtx* ctx, size_t size) {
    // Same as block_alloc_aligned but works with the active buffer
    // Mesmo que block_alloc_aligned mas funciona com o buffer ativo
    // No spin-wait for frozen flag needed — double-buffer eliminates the pause
    // Sem espera ocupada para flag frozen — double-buffer elimina a pausa
    size_t alignment = size >= 8 ? (size_t)8
                    : size >= 4 ? (size_t)4
                    : size >= 2 ? (size_t)2
                    :             (size_t)1;

    size_t current = ctx->used;
    size_t aligned = (current + alignment - 1) & ~(alignment - 1);

    if (aligned + size > ctx->capacity) {
        error("block overflow: out of memory in block");
    }

    ctx->used = aligned + size;
    ctx->allocation_count++;

    if (ctx->used > ctx->peak_used) {
        ctx->peak_used = ctx->used;
    }

    return ctx->data + aligned;
}

int block_has_double_buffer(BlockCtx* ctx) {
    return find_db_ext(ctx) != NULL;
}

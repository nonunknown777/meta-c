#include "embedded_runtime.h"

namespace brick { namespace embedded {

const char _runtime_block_memory_h[] = R"(
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
)";
const size_t _runtime_block_memory_h_len = sizeof(_runtime_block_memory_h) - 1;

const char _runtime_block_memory_c[] = R"X(
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
)X";
const size_t _runtime_block_memory_c_len = sizeof(_runtime_block_memory_c) - 1;

const char _runtime_io_h[] = R"(
#ifndef BRICK_IO_H
#define BRICK_IO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char*  data;
    size_t len;
} BrickString;

void io_print_u8(uint8_t val);
void io_print_u16(uint16_t val);
void io_print_u32(uint32_t val);
void io_print_u64(uint64_t val);
void io_print_i8(int8_t val);
void io_print_i16(int16_t val);
void io_print_i32(int32_t val);
void io_print_i64(int64_t val);
void io_print_f32(float val);
void io_print_f64(double val);
void io_print_usize(size_t val);
void io_print_isize(ptrdiff_t val);
void io_print_int(int64_t val);
void io_print_float(double val);
void io_print_string(const char* data, size_t len);
void io_print_char(char val);
void io_print_bool(uint8_t val);
void io_print_newline(void);
void io_printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // BRICK_IO_H
     // BRICK_IO_H
)";
const size_t _runtime_io_h_len = sizeof(_runtime_io_h) - 1;

const char _runtime_io_c[] = R"(
#include "io.h"
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

// ─── Type-specific integer prints ─────────────────────────────
// ─── Prints de inteiro especificos por tipo ────────────────────

void io_print_u8(uint8_t v)     { printf("%" PRIu8 "\n", v); }
void io_print_u16(uint16_t v)   { printf("%" PRIu16 "\n", v); }
void io_print_u32(uint32_t v)   { printf("%" PRIu32 "\n", v); }
void io_print_u64(uint64_t v)   { printf("%" PRIu64 "\n", v); }
void io_print_i8(int8_t v)      { printf("%" PRId8 "\n", v); }
void io_print_i16(int16_t v)    { printf("%" PRId16 "\n", v); }
void io_print_i32(int32_t v)    { printf("%" PRId32 "\n", v); }
void io_print_i64(int64_t v)    { printf("%" PRId64 "\n", v); }
void io_print_usize(size_t v)   { printf("%zu\n", v); }
void io_print_isize(ptrdiff_t v){ printf("%td\n", v); }

// ─── Type-specific float prints ───────────────────────────────
// ─── Prints de ponto flutuante especificos por tipo ────────────

void io_print_f32(float v)      { printf("%f\n", (double)v); }
void io_print_f64(double v)     { printf("%f\n", v); }

// ─── Generic prints (kept for backward compat) ────────────────
// ─── Prints genericos (mantidos pra compatibilidade) ───────────

void io_print_int(int64_t v)    { io_print_i64(v); }
void io_print_float(double v)   { io_print_f64(v); }

void io_print_string(const char* data, size_t len) {
    printf("%.*s\n", (int)len, data);
}

void io_print_bool(uint8_t v) {
    printf("%s\n", v ? "true" : "false");
}

void io_print_char(char v) {
    printf("%c\n", v);
}

void io_print_newline(void) {
    printf("\n");
}

void io_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
)";
const size_t _runtime_io_c_len = sizeof(_runtime_io_c) - 1;

const char _runtime_hot_reload_h[] = R"(
#ifndef BRICK_HOT_RELOAD_H
#define BRICK_HOT_RELOAD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HR_WAITING,
    HR_LOADING,
    HR_OK,
    HR_ERROR,
} HotReloadState;

typedef struct HotReloadEngine HotReloadEngine;

// Function pointer type for reload callbacks
// Tipo do ponteiro de funcao para callbacks de reload
typedef void (*hr_callback_t)(const char* so_path);

// Create a hot reload engine for a .so library
// Cria um motor de hot reload para uma biblioteca .so
HotReloadEngine* hr_create(const char* so_path);

// Load initial symbols from the .so
// Carrega simbolos iniciais do .so
int hr_load_initial(HotReloadEngine* hr);

// Register a function pointer for hot swapping
// Registra um ponteiro de funcao para troca a quente
// name: symbol name in the .so
// name: nome do simbolo no .so
// func_ptr: address of a void* that will be updated on reload
// func_ptr: endereco de um void* que sera atualizado no reload
int hr_register_func(HotReloadEngine* hr, const char* name, void** func_ptr);

// Start watching the .so file for modifications (inotify)
// Inicia monitoramento do arquivo .so por modificacoes (inotify)
// Creates a separate monitoring thread
// Cria uma thread de monitoramento separada
int hr_start_watching(HotReloadEngine* hr);

// Force a reload immediately
// Forca um reload imediatamente
int hr_reload(HotReloadEngine* hr);

// Get current state
// Obtem estado atual
HotReloadState hr_state(HotReloadEngine* hr);

// Set a callback that fires after every reload attempt
// Define um callback que executa apos cada tentativa de reload
void hr_set_callback(HotReloadEngine* hr, hr_callback_t cb);

// Stop monitoring and cleanup
// Para monitoramento e limpa
void hr_destroy(HotReloadEngine* hr);

#ifdef __cplusplus
}
#endif

#endif // BRICK_HOT_RELOAD_H
     // BRICK_HOT_RELOAD_H
)";
const size_t _runtime_hot_reload_h_len = sizeof(_runtime_hot_reload_h) - 1;

const char _runtime_libs_window_window_h[] = R"(
#ifndef BRICK_WINDOW_H
#define BRICK_WINDOW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration — full type in block_memory.h */
typedef struct BlockCtx BlockCtx;

/* ─── Window creation flags ─── */

typedef enum {
    META_WINDOW_NONE       = 0,
    META_WINDOW_RESIZABLE  = 1 << 0,
    META_WINDOW_FULLSCREEN = 1 << 1,
    META_WINDOW_HIDDEN     = 1 << 2,
    META_WINDOW_BORDERLESS = 1 << 3,
    META_WINDOW_VSYNC      = 1 << 4,
} MetaWindowFlags;

/* ─── Event types ─── */

typedef enum {
    META_EVENT_NONE        = 0,
    META_EVENT_CLOSE       = 1,
    META_EVENT_RESIZE      = 2,
    META_EVENT_KEY_DOWN    = 3,
    META_EVENT_KEY_UP      = 4,
    META_EVENT_MOUSE_MOVE  = 5,
    META_EVENT_MOUSE_BTN   = 6,
    META_EVENT_MOUSE_WHEEL = 7,
} MetaEventType;

typedef struct {
    MetaEventType type;
    int64_t       timestamp_ms;
    union {
        struct { int width, height; }          resize;
        struct { int keycode, mods; }          key;
        struct { double x, y; }                mouse_move;
        struct { int button, action, mods; }   mouse_btn;
        struct { double delta; }               mouse_wheel;
    } data;
} MetaEvent;

/* ─── Window handle ─── */

typedef struct MetaWindowImpl MetaWindow;

/**
 * Creates a window with the given title, dimensions, and flags.
 *
 * The MetaWindow struct is allocated from the given BlockCtx.
 * Call meta_window_destroy() to close the native window and release
 * OS resources, then reset or destroy the block to reclaim the memory.
 *
 * Returns NULL on failure.
 *
 * Example (Brick):
 *   using Window
 *   block win = 1MB
 *   Window w = Window.create("Hello", 800, 600, Window.RESIZABLE | Window.VSYNC) @win
 *
 * Example (C):
 *   BlockCtx* block = block_create_bytes(65536);
 *   MetaWindow* w = meta_window_create(block, "Hello", 800, 600, META_WINDOW_VSYNC);
 */
MetaWindow* meta_window_create(
    BlockCtx* block,
    const char* title,
    int width, int height,
    uint32_t flags
);

/**
 * Destroys the native window and releases OS resources (X11/Win32).
 * Does NOT free the MetaWindow struct — that memory belongs to the
 * BlockCtx passed at creation. Call block_reset() or block_destroy()
 * to reclaim the memory.
 */
void meta_window_destroy(MetaWindow* w);

/**
 * Polls all pending events. Returns the number of events enqueued.
 * Call meta_window_next_event() to retrieve them one by one.
 */
int meta_window_poll_events(MetaWindow* w);

/**
 * Retrieves the next pending event. Returns 0 if no more events.
 * Must be called in a loop after meta_window_poll_events().
 */
int meta_window_next_event(MetaWindow* w, MetaEvent* out);

/**
 * Swaps the front and back buffers. Call after rendering each frame.
 */
void meta_window_swap_buffers(MetaWindow* w);

/**
 * Returns non-zero if the close button has been pressed.
 */
int meta_window_should_close(MetaWindow* w);

/**
 * Sets the window title at runtime.
 */
void meta_window_set_title(MetaWindow* w, const char* title);

/**
 * Returns the current window dimensions.
 */
void meta_window_get_size(MetaWindow* w, int* out_width, int* out_height);

/**
 * Toggles fullscreen mode.
 */
void meta_window_set_fullscreen(MetaWindow* w, int fullscreen);

/**
 * Returns the native window handle (Window on X11, HWND on Win32).
 * Useful for advanced interop. Use sparingly.
 */
void* meta_window_native_handle(MetaWindow* w);

#ifdef __cplusplus
}
#endif

#endif /* BRICK_WINDOW_H */
)";
const size_t _runtime_libs_window_window_h_len = sizeof(_runtime_libs_window_window_h) - 1;

const char _runtime_libs_window_window_internal_h[] = R"(
#ifndef BRICK_WINDOW_INTERNAL_H
#define BRICK_WINDOW_INTERNAL_H

#include "window.h"
#include "../../block_memory.h"

/* ─── Event queue ring buffer ─── */
#define META_WINDOW_EVENT_CAPACITY 256

typedef struct {
    MetaEvent  events[META_WINDOW_EVENT_CAPACITY];
    int        head;
    int        tail;
    int        count;
} EventQueue;

static inline void event_queue_init(EventQueue* q) {
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}

static inline int event_queue_push(EventQueue* q, const MetaEvent* e) {
    if (q->count >= META_WINDOW_EVENT_CAPACITY) return 0;
    q->events[q->tail] = *e;
    q->tail = (q->tail + 1) % META_WINDOW_EVENT_CAPACITY;
    q->count++;
    return 1;
}

static inline int event_queue_pop(EventQueue* q, MetaEvent* out) {
    if (q->count == 0) return 0;
    *out = q->events[q->head];
    q->head = (q->head + 1) % META_WINDOW_EVENT_CAPACITY;
    q->count--;
    return 1;
}

/* ─── Platform-specific state ─── */

struct MetaWindowImpl {
    /* Shared */
    EventQueue   event_queue;
    char         title[256];
    int          width;
    int          height;
    uint32_t     flags;
    int          should_close;

    /* Block allocator context for this window's data */
    BlockCtx*    block;

#if defined(__linux__)
    /* X11 state */
    void*        display;   /* Display* */
    unsigned long window;   /* Window */
    unsigned long gc;       /* GC */
#elif defined(_WIN32)
    /* Win32 state */
    void*        hinstance; /* HINSTANCE */
    void*        hwnd;      /* HWND */
    void*        hdc;       /* HDC */
#endif
};

#endif /* BRICK_WINDOW_INTERNAL_H */
)";
const size_t _runtime_libs_window_window_internal_h_len = sizeof(_runtime_libs_window_window_internal_h) - 1;

const char _runtime_libs_window_window_hr_h[] = R"(
#ifndef BRICK_WINDOW_HR_H
#define BRICK_WINDOW_HR_H

#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Hot-reloadable function pointer table ─── */

typedef struct {
    void* create;
    void* destroy;
    void* poll_events;
    void* next_event;
    void* swap_buffers;
    void* should_close;
    void* set_title;
    void* get_size;
    void* set_fullscreen;
    void* native_handle;
} MetaWindowFuncTable;

/* Global table — populated by meta_window_hr_init().
 * On hot reload the entries are atomically swapped. */
extern MetaWindowFuncTable meta_window_table;

/* ─── Type-safe inline wrappers ─── */

static inline MetaWindow* meta_win_create(
    BlockCtx* block, const char* title, int width, int height, uint32_t flags
) {
    MetaWindow* (*fn)(BlockCtx*, const char*, int, int, uint32_t) =
        (MetaWindow* (*)(BlockCtx*, const char*, int, int, uint32_t))meta_window_table.create;
    return fn(block, title, width, height, flags);
}

static inline void meta_win_destroy(MetaWindow* w) {
    void (*fn)(MetaWindow*) = (void (*)(MetaWindow*))meta_window_table.destroy;
    fn(w);
}

static inline int meta_win_poll_events(MetaWindow* w) {
    int (*fn)(MetaWindow*) = (int (*)(MetaWindow*))meta_window_table.poll_events;
    return fn(w);
}

static inline int meta_win_next_event(MetaWindow* w, MetaEvent* out) {
    int (*fn)(MetaWindow*, MetaEvent*) =
        (int (*)(MetaWindow*, MetaEvent*))meta_window_table.next_event;
    return fn(w, out);
}

static inline void meta_win_swap_buffers(MetaWindow* w) {
    void (*fn)(MetaWindow*) = (void (*)(MetaWindow*))meta_window_table.swap_buffers;
    fn(w);
}

static inline int meta_win_should_close(MetaWindow* w) {
    int (*fn)(MetaWindow*) = (int (*)(MetaWindow*))meta_window_table.should_close;
    return fn(w);
}

static inline void meta_win_set_title(MetaWindow* w, const char* title) {
    void (*fn)(MetaWindow*, const char*) =
        (void (*)(MetaWindow*, const char*))meta_window_table.set_title;
    fn(w, title);
}

static inline void meta_win_get_size(MetaWindow* w, int* out_w, int* out_h) {
    void (*fn)(MetaWindow*, int*, int*) =
        (void (*)(MetaWindow*, int*, int*))meta_window_table.get_size;
    fn(w, out_w, out_h);
}

static inline void meta_win_set_fullscreen(MetaWindow* w, int fullscreen) {
    void (*fn)(MetaWindow*, int) =
        (void (*)(MetaWindow*, int))meta_window_table.set_fullscreen;
    fn(w, fullscreen);
}

static inline void* meta_win_native_handle(MetaWindow* w) {
    void* (*fn)(MetaWindow*) = (void* (*)(MetaWindow*))meta_window_table.native_handle;
    return fn(w);
}

/* ─── Forward declaration of HotReloadEngine ─── */

struct HotReloadEngine;

/* ─── Initialization ─── */

/* Register all window symbols with an existing HotReloadEngine.
 * Call hr_load_initial() afterwards, or just use this which does it.
 * Returns 0 on success, -1 on failure. */
int meta_window_hr_init(struct HotReloadEngine* hr);

/* Convenience: creates a HotReloadEngine, registers all symbols,
 * loads initial, and starts watching the .so at so_path.
 * Returns the engine on success, NULL on failure.
 * The caller owns the engine and must call hr_destroy() on shutdown. */
struct HotReloadEngine* meta_window_hr_start(const char* so_path);

#ifdef __cplusplus
}
#endif

#endif /* BRICK_WINDOW_HR_H */
)";
const size_t _runtime_libs_window_window_hr_h_len = sizeof(_runtime_libs_window_window_hr_h) - 1;

const char _runtime_hot_reload_c[] = R"(
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L
#include "hot_reload.h"
#include "block_memory.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <stdatomic.h>

#define MAX_SYMBOLS 256
#define EVENT_BUF_LEN 4096

typedef struct {
    char  name[128];
    void** func_ptr;
} SymbolEntry;

struct HotReloadEngine {
    char          so_path[1024];
    char          so_basename[256];
    char          so_path_tmp[1024];  // path + ".new" for atomic swap
                                       // path + ".new" para troca atomica
    void*         handle_current;
    void*         handle_new;
    SymbolEntry   symbols[MAX_SYMBOLS];
    int           symbol_count;
    atomic_int    state;
    pthread_t     watch_thread;
    int           inotify_fd;
    int           watch_fd;
    volatile int  running;
    hr_callback_t callback;
    pthread_mutex_t mutex;
};

static void error(const char* msg) {
    fprintf(stderr, "Brick hot reload error: %s\n", msg);
    exit(1);
}

HotReloadEngine* hr_create(const char* so_path) {
    HotReloadEngine* hr = (HotReloadEngine*)calloc(1, sizeof(HotReloadEngine));
    if (!hr) error("out of memory");

    strncpy(hr->so_path, so_path, sizeof(hr->so_path) - 1);

    // Extract basename
    // Extrai o nome base
    const char* slash = strrchr(so_path, '/');
    strncpy(hr->so_basename, slash ? slash + 1 : so_path, sizeof(hr->so_basename) - 1);

    snprintf(hr->so_path_tmp, sizeof(hr->so_path_tmp), "%s.new", so_path);

    hr->state = HR_WAITING;
    hr->running = 0;
    hr->callback = NULL;
    hr->inotify_fd = -1;
    hr->watch_fd = -1;
    pthread_mutex_init(&hr->mutex, NULL);

    return hr;
}

int hr_load_initial(HotReloadEngine* hr) {
    pthread_mutex_lock(&hr->mutex);

    hr->handle_current = dlopen(hr->so_path, RTLD_NOW | RTLD_LOCAL);
    if (!hr->handle_current) {
        fprintf(stderr, "hr: dlopen failed: %s\n", dlerror());
        hr->state = HR_ERROR;
        pthread_mutex_unlock(&hr->mutex);
        return -1;
    }

    hr->state = HR_OK;

    // Resolve registered symbols
    // Resolve simbolos registrados
    for (int i = 0; i < hr->symbol_count; i++) {
        void* sym = dlsym(hr->handle_current, hr->symbols[i].name);
        if (sym) {
            *(hr->symbols[i].func_ptr) = sym;
        } else {
            fprintf(stderr, "hr: symbol '%s' not found\n", hr->symbols[i].name);
        }
    }

    pthread_mutex_unlock(&hr->mutex);
    return 0;
}

int hr_register_func(HotReloadEngine* hr, const char* name, void** func_ptr) {
    if (hr->symbol_count >= MAX_SYMBOLS) return -1;

    strncpy(hr->symbols[hr->symbol_count].name, name,
            sizeof(hr->symbols[hr->symbol_count].name) - 1);
    hr->symbols[hr->symbol_count].func_ptr = func_ptr;
    hr->symbol_count++;
    return 0;
}

int hr_reload(HotReloadEngine* hr) {
    pthread_mutex_lock(&hr->mutex);
    hr->state = HR_LOADING;

    // Freeze block allocations during swap
    // Congela alocacoes de bloco durante a troca
    block_freeze();

    // Copy the .so to a temp path so dlopen gets a fresh load
    // Copia o .so para um caminho temporario para dlopen obter carga fresca
    // (dlopen caches by path; a copy bypasses the cache)
    // (dlopen armazena em cache por caminho; uma copia contorna o cache)
    char tmp_path[1088];
    snprintf(tmp_path, sizeof(tmp_path), "%s.%d", hr->so_path, (int)time(NULL));

    FILE* src = fopen(hr->so_path, "rb");
    void* new_handle = NULL;
    if (src) {
        FILE* dst = fopen(tmp_path, "wb");
        if (dst) {
            char buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
                fwrite(buf, 1, n, dst);
            fclose(dst);

            new_handle = dlopen(tmp_path, RTLD_NOW | RTLD_LOCAL);
            unlink(tmp_path);
        }
        fclose(src);
    }

    if (!new_handle) {
        const char* err = dlerror();
        fprintf(stderr, "hr: reload failed: %s\n", err ? err : "cannot open .so file");
        // Rollback: keep old handle, function pointers untouched
        // Rollback: mantem handle antigo, ponteiros de funcao intocados
        hr->state = hr->handle_current ? HR_OK : HR_ERROR;
        block_thaw();
        if (hr->callback) hr->callback(hr->so_path);
        pthread_mutex_unlock(&hr->mutex);
        return -1;
    }

    // Swap function pointers atomically
    // Troca ponteiros de funcao atomicamente
    for (int i = 0; i < hr->symbol_count; i++) {
        void* sym = dlsym(new_handle, hr->symbols[i].name);
        if (sym) {
            __atomic_store(hr->symbols[i].func_ptr, &sym, __ATOMIC_SEQ_CST);
        } else {
            fprintf(stderr, "hr: symbol '%s' not found in new .so\n",
                    hr->symbols[i].name);
        }
    }

    // Close old handle
    // Fecha handle antigo
    if (hr->handle_current) {
        dlclose(hr->handle_current);
    }
    hr->handle_current = new_handle;
    hr->state = HR_OK;

    block_thaw();
    if (hr->callback) hr->callback(hr->so_path);

    pthread_mutex_unlock(&hr->mutex);
    return 0;
}

// Watch thread function
// Funcao da thread de monitoramento
static void* watch_thread_fn(void* arg) {
    HotReloadEngine* hr = (HotReloadEngine*)arg;

    char buf[EVENT_BUF_LEN] __attribute__((aligned(8)));

    while (hr->running) {
        ssize_t len = read(hr->inotify_fd, buf, sizeof(buf));
        if (len <= 0) continue;

        size_t i = 0;
        while (i < (size_t)len) {
            struct inotify_event* event = (struct inotify_event*)&buf[i];

            if ((event->mask & IN_CLOSE_WRITE) &&
                event->len > 0 &&
                strcmp(event->name, hr->so_basename) == 0) {
                // Small delay to let file writes complete
                // Pequeno atraso para permitir que escritas no arquivo terminem
                struct timespec ts = {0, 50000000};
                nanosleep(&ts, NULL);
                hr_reload(hr);
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

    return NULL;
}

int hr_start_watching(HotReloadEngine* hr) {
    hr->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (hr->inotify_fd < 0) {
        perror("inotify_init");
        return -1;
    }

    // Watch directory of the .so file
    // Monitora diretorio do arquivo .so
    char dir_path[1024];
    size_t path_len = strlen(hr->so_path);
    if (path_len >= sizeof(dir_path)) path_len = sizeof(dir_path) - 1;
    memcpy(dir_path, hr->so_path, path_len);
    dir_path[path_len] = '\0';
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) *last_slash = '\0';
    else strcpy(dir_path, ".");

    hr->watch_fd = inotify_add_watch(hr->inotify_fd, dir_path, IN_CLOSE_WRITE);
    if (hr->watch_fd < 0) {
        perror("inotify_add_watch");
        return -1;
    }

    hr->running = 1;
    if (pthread_create(&hr->watch_thread, NULL, watch_thread_fn, hr) != 0) {
        perror("pthread_create");
        hr->running = 0;
        return -1;
    }

    return 0;
}

HotReloadState hr_state(HotReloadEngine* hr) {
    return (HotReloadState)atomic_load(&hr->state);
}

void hr_set_callback(HotReloadEngine* hr, hr_callback_t cb) {
    hr->callback = cb;
}

void hr_destroy(HotReloadEngine* hr) {
    if (hr->running) {
        hr->running = 0;
        pthread_join(hr->watch_thread, NULL);
    }

    if (hr->inotify_fd >= 0) close(hr->inotify_fd);
    if (hr->handle_current) dlclose(hr->handle_current);
    if (hr->handle_new) dlclose(hr->handle_new);

    pthread_mutex_destroy(&hr->mutex);
    free(hr);
}
)";
const size_t _runtime_hot_reload_c_len = sizeof(_runtime_hot_reload_c) - 1;

const char _runtime_libs_window_window_linux_c[] = R"(
/**
 * window_linux.c — X11 backend for Brick Window library
 *
 * Uses Xlib directly (no XCB) for lower overhead and simpler sync.
 * The event queue is allocated from the Brick block allocator.
 */

#include "window_internal.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Reuse io.h's printf-style logging — or we add a simple log macro. */
#include "../../io.h"

/* ─── Key mapping helpers ─── */

static int x11_keycode_to_meta(KeySym ks) {
    switch (ks) {
        case XK_Escape:    return 256;
        case XK_Return:    return 257;
        case XK_Tab:       return 258;
        case XK_Left:      return 263;
        case XK_Right:     return 262;
        case XK_Up:        return 265;
        case XK_Down:      return 264;
        case XK_Shift_L:
        case XK_Shift_R:   return 340;
        case XK_Control_L:
        case XK_Control_R: return 341;
        case XK_Alt_L:
        case XK_Alt_R:     return 342;
        case XK_space:     return 32;
        default:
            if (ks >= XK_a && ks <= XK_z) return (int)ks - XK_a + 97;
            if (ks >= XK_0 && ks <= XK_9) return (int)ks;
            return 0;
    }
}

/* ─── Creation ─── */

MetaWindow* meta_window_create(
    BlockCtx* block, const char* title, int width, int height, uint32_t flags
) {
    if (!block) return NULL;

    MetaWindow* w = (MetaWindow*)block_alloc(block, sizeof(MetaWindow));
    if (!w) return NULL;

    event_queue_init(&w->event_queue);
    w->width  = width;
    w->height = height;
    w->flags  = flags;
    w->block  = block;

    if (title) {
        size_t len = strlen(title);
        if (len >= sizeof(w->title)) len = sizeof(w->title) - 1;
        memcpy(w->title, title, len);
        w->title[len] = '\0';
    } else {
        memcpy(w->title, "Brick", 7);
    }

    /* Open X display */
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) {
        io_print_string("FATAL: Cannot open X display", 28);
        return NULL;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    /* Window attributes */
    XSetWindowAttributes attrs;
    attrs.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask | FocusChangeMask;

    Window xwin = XCreateWindow(
        dpy, root,
        0, 0, width, height, 0,
        CopyFromParent, InputOutput,
        CopyFromParent,
        CWEventMask, &attrs
    );

    /* Set title via WM hints */
    XStoreName(dpy, xwin, w->title);
    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, xwin, &wm_delete, 1);

    /* Resizability */
    if (!(flags & META_WINDOW_RESIZABLE)) {
        XSizeHints hints;
        hints.flags = PMinSize | PMaxSize;
        hints.min_width  = width;
        hints.max_width  = width;
        hints.min_height = height;
        hints.max_height = height;
        XSetWMNormalHints(dpy, xwin, &hints);
    }

    /* Fullscreen */
    if (flags & META_WINDOW_FULLSCREEN) {
        Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom full = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
        XChangeProperty(dpy, xwin, wm_state, XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&full, 1);
    }

    GC gc = XCreateGC(dpy, xwin, 0, NULL);

    /* Sync before storing */
    XMapWindow(dpy, xwin);
    XFlush(dpy);

    w->display = (void*)dpy;
    w->window  = (unsigned long)xwin;
    w->gc      = (unsigned long)gc;

    return w;
}

/* ─── Destruction ─── */

void meta_window_destroy(MetaWindow* w) {
    if (!w) return;
    Display* dpy = (Display*)w->display;
    if (dpy) {
        XFreeGC(dpy, (GC)w->gc);
        XDestroyWindow(dpy, (Window)w->window);
        XCloseDisplay(dpy);
    }
    w->display = NULL;
}

/* ─── Event polling ─── */

int meta_window_poll_events(MetaWindow* w) {
    if (!w) return 0;
    Display* dpy = (Display*)w->display;
    int count = 0;

    /* Process all pending X11 events */
    XEvent xe;
    while (XPending(dpy)) {
        XNextEvent(dpy, &xe);

        MetaEvent me;
        memset(&me, 0, sizeof(me));
        me.timestamp_ms = 0; /* X11 timestamps are server-time; skip for now */

        switch (xe.type) {
            case ClientMessage: {
                Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
                if ((Atom)xe.xclient.data.l[0] == wm_delete) {
                    me.type = META_EVENT_CLOSE;
                    w->should_close = 1;
                }
                break;
            }
            case DestroyNotify:
                me.type = META_EVENT_CLOSE;
                w->should_close = 1;
                break;
            case ConfigureNotify: {
                int new_w = xe.xconfigure.width;
                int new_h = xe.xconfigure.height;
                if (new_w != w->width || new_h != w->height) {
                    w->width  = new_w;
                    w->height = new_h;
                    me.type = META_EVENT_RESIZE;
                    me.data.resize.width  = new_w;
                    me.data.resize.height = new_h;
                }
                break;
            }
            case KeyPress: {
                KeySym ks = XLookupKeysym(&xe.xkey, 0);
                me.type = META_EVENT_KEY_DOWN;
                me.data.key.keycode = x11_keycode_to_meta(ks);
                me.data.key.mods    = xe.xkey.state;
                break;
            }
            case KeyRelease: {
                KeySym ks = XLookupKeysym(&xe.xkey, 0);
                me.type = META_EVENT_KEY_UP;
                me.data.key.keycode = x11_keycode_to_meta(ks);
                me.data.key.mods    = xe.xkey.state;
                break;
            }
            case ButtonPress:
            case ButtonRelease: {
                if (xe.xbutton.button >= 4 && xe.xbutton.button <= 7) {
                    /* Scroll wheel */
                    me.type = META_EVENT_MOUSE_WHEEL;
                    me.data.mouse_wheel.delta =
                        (xe.xbutton.button == 4 || xe.xbutton.button == 6) ? 1.0 : -1.0;
                } else {
                    me.type = (xe.type == ButtonPress)
                              ? META_EVENT_MOUSE_BTN
                              : META_EVENT_MOUSE_BTN;
                    me.data.mouse_btn.button = (int)xe.xbutton.button;
                    me.data.mouse_btn.action = (xe.type == ButtonPress) ? 1 : 0;
                    me.data.mouse_btn.mods   = xe.xbutton.state;
                }
                break;
            }
            case MotionNotify:
                me.type = META_EVENT_MOUSE_MOVE;
                me.data.mouse_move.x = xe.xmotion.x;
                me.data.mouse_move.y = xe.xmotion.y;
                break;
            default:
                continue; /* skip unhandled */
        }

        if (me.type != META_EVENT_NONE) {
            event_queue_push(&w->event_queue, &me);
            count++;
        }
    }

    return count;
}

int meta_window_next_event(MetaWindow* w, MetaEvent* out) {
    if (!w || !out) return 0;
    return event_queue_pop(&w->event_queue, out);
}

/* ─── Swap buffers ─── */

void meta_window_swap_buffers(MetaWindow* w) {
    if (!w) return;
    Display* dpy = (Display*)w->display;
    XFlush(dpy);
}

/* ─── Queries ─── */

int meta_window_should_close(MetaWindow* w) {
    return w ? w->should_close : 1;
}

void meta_window_set_title(MetaWindow* w, const char* title) {
    if (!w || !title) return;
    size_t len = strlen(title);
    if (len >= sizeof(w->title)) len = sizeof(w->title) - 1;
    memcpy(w->title, title, len);
    w->title[len] = '\0';
    Display* dpy = (Display*)w->display;
    if (dpy) XStoreName(dpy, (Window)w->window, w->title);
}

void meta_window_get_size(MetaWindow* w, int* out_w, int* out_h) {
    if (!w) return;
    if (out_w) *out_w = w->width;
    if (out_h) *out_h = w->height;
}

void meta_window_set_fullscreen(MetaWindow* w, int fullscreen) {
    if (!w) return;
    Display* dpy = (Display*)w->display;
    if (!dpy) return;
    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom full     = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    XEvent xe;
    memset(&xe, 0, sizeof(xe));
    xe.type = ClientMessage;
    xe.xclient.window = (Window)w->window;
    xe.xclient.message_type = wm_state;
    xe.xclient.format = 32;
    xe.xclient.data.l[0] = fullscreen ? 1 : 0; /* _NET_WM_STATE_ADD / _REMOVE */
    xe.xclient.data.l[1] = (long)full;
    XSendEvent(dpy, DefaultRootWindow(dpy), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xe);
}

void* meta_window_native_handle(MetaWindow* w) {
    if (!w) return NULL;
    return (void*)(uintptr_t)w->window;
}
)";
const size_t _runtime_libs_window_window_linux_c_len = sizeof(_runtime_libs_window_window_linux_c) - 1;

const char _runtime_libs_window_window_hr_c[] = R"(
/**
 * window_hr.c — Hot reload support for Brick Window library
 *
 * Defines the global function pointer table and hooks into the
 * HotReloadEngine so that all window API calls can be swapped
 * atomically at runtime when the .so is recompiled.
 */

#include "window_hr.h"
#include "../../hot_reload.h"

/* ─── Global function table ─── */

MetaWindowFuncTable meta_window_table = { NULL };

/* ─── Registration ─── */

int meta_window_hr_init(HotReloadEngine* hr) {
    if (!hr) return -1;

    hr_register_func(hr, "meta_window_create",       &meta_window_table.create);
    hr_register_func(hr, "meta_window_destroy",       &meta_window_table.destroy);
    hr_register_func(hr, "meta_window_poll_events",   &meta_window_table.poll_events);
    hr_register_func(hr, "meta_window_next_event",    &meta_window_table.next_event);
    hr_register_func(hr, "meta_window_swap_buffers",  &meta_window_table.swap_buffers);
    hr_register_func(hr, "meta_window_should_close",  &meta_window_table.should_close);
    hr_register_func(hr, "meta_window_set_title",     &meta_window_table.set_title);
    hr_register_func(hr, "meta_window_get_size",      &meta_window_table.get_size);
    hr_register_func(hr, "meta_window_set_fullscreen",&meta_window_table.set_fullscreen);
    hr_register_func(hr, "meta_window_native_handle", &meta_window_table.native_handle);

    return hr_load_initial(hr);
}

/* ─── Convenience: create, init, start watching ─── */

HotReloadEngine* meta_window_hr_start(const char* so_path) {
    HotReloadEngine* hr = hr_create(so_path);
    if (!hr) return NULL;

    if (meta_window_hr_init(hr) != 0) {
        hr_destroy(hr);
        return NULL;
    }

    if (hr_start_watching(hr) != 0) {
        hr_destroy(hr);
        return NULL;
    }

    return hr;
}
)";
const size_t _runtime_libs_window_window_hr_c_len = sizeof(_runtime_libs_window_window_hr_c) - 1;

const EmbeddedFile _runtime_sources[] = {
    {"block_memory.h", _runtime_block_memory_h, _runtime_block_memory_h_len},
    {"block_memory.c", _runtime_block_memory_c, _runtime_block_memory_c_len},
    {"io.h", _runtime_io_h, _runtime_io_h_len},
    {"io.c", _runtime_io_c, _runtime_io_c_len},
    {"hot_reload.h", _runtime_hot_reload_h, _runtime_hot_reload_h_len},
    {"libs/window/window.h", _runtime_libs_window_window_h, _runtime_libs_window_window_h_len},
    {"libs/window/window_internal.h", _runtime_libs_window_window_internal_h, _runtime_libs_window_window_internal_h_len},
    {"libs/window/window_hr.h", _runtime_libs_window_window_hr_h, _runtime_libs_window_window_hr_h_len},
    {"hot_reload.c", _runtime_hot_reload_c, _runtime_hot_reload_c_len},
    {"libs/window/window_linux.c", _runtime_libs_window_window_linux_c, _runtime_libs_window_window_linux_c_len},
    {"libs/window/window_hr.c", _runtime_libs_window_window_hr_c, _runtime_libs_window_window_hr_c_len},
};

const char* _runtime_flags[] = {
    "-ldl",
    "-lpthread",
    "-lX11",
};

const RuntimeInfo runtime_info = {
    "linux",
    "gcc",
    11,
    _runtime_sources,
    3,
    _runtime_flags,
};

} }

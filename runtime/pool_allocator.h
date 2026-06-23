#ifndef BRICK_POOL_ALLOCATOR_H
#define BRICK_POOL_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fixed-size block pool allocator for small objects (like String, small structs)
// Alocador de pool de tamanho fixo para objetos pequenos (como String, structs pequenas)
// Much faster than malloc for many small allocations — O(1) alloc and free
// Muito mais rapido que malloc para muitas alocacoes pequenas — O(1) alloc e free

#define POOL_MAX_SLOTS 8

typedef struct PoolSlot {
    size_t   block_size;     // size of each block in this slot
    size_t   capacity;       // total number of blocks
    size_t   count;          // number of free blocks
    void**   free_list;      // linked list of free blocks
    uint8_t* data;           // contiguous memory pool
} PoolSlot;

typedef struct {
    PoolSlot slots[POOL_MAX_SLOTS];
    int      slot_count;
} PoolAllocator;

// Create a pool with a fixed block size and count
// Cria um pool com tamanho de bloco fixo e contagem
PoolAllocator* pool_create(void);

// Add a slot with a given block size and count
// Adiciona um slot com um dado tamanho de bloco e contagem
// Returns slot index or -1 on failure
// Retorna indice do slot ou -1 em falha
int pool_add_slot(PoolAllocator* pool, size_t block_size, size_t count);

// Allocate a block from the smallest slot that fits the requested size
// Aloca um bloco do menor slot que cabe o tamanho solicitado
void* pool_alloc(PoolAllocator* pool, size_t size);

// Return a block to the pool
// Retorna um bloco ao pool
void pool_free(PoolAllocator* pool, void* ptr);

// Destroy the pool and free all memory
// Destroi o pool e libera toda a memoria
void pool_destroy(PoolAllocator* pool);

// Get stats
typedef struct {
    size_t block_size;
    size_t capacity;
    size_t used;
    size_t free;
} PoolStats;

PoolStats pool_slot_stats(PoolAllocator* pool, int slot);

#ifdef __cplusplus
}
#endif

#endif

#define _GNU_SOURCE
#include "pool_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    PoolAllocator* pool;
    int slot;
} PoolBlockHeader;

static void error(const char* msg) {
    fprintf(stderr, "Brick pool allocator error: %s\n", msg);
    exit(1);
}

PoolAllocator* pool_create(void) {
    PoolAllocator* pool = (PoolAllocator*)calloc(1, sizeof(PoolAllocator));
    if (!pool) error("out of memory");
    pool->slot_count = 0;
    return pool;
}

int pool_add_slot(PoolAllocator* pool, size_t block_size, size_t count) {
    if (pool->slot_count >= POOL_MAX_SLOTS) return -1;

    int idx = pool->slot_count;
    PoolSlot* slot = &pool->slots[idx];
    slot->block_size = block_size;
    slot->capacity = count;
    slot->count = count;

    // Allocate contiguous data + free list
    // Aloca dados contiguos + lista livre
    size_t total = count * block_size;
    slot->data = (uint8_t*)calloc(1, total);
    if (!slot->data) return -1;

    slot->free_list = (void**)calloc(count, sizeof(void*));
    if (!slot->free_list) {
        free(slot->data);
        return -1;
    }

    // Initialize free list: each entry points to a block
    // Inicializa lista livre: cada entrada aponta para um bloco
    for (size_t i = 0; i < count; i++) {
        slot->free_list[i] = slot->data + (i * block_size);
    }

    pool->slot_count++;
    return idx;
}

void* pool_alloc(PoolAllocator* pool, size_t size) {
    // Find the smallest slot that fits the requested size + header
    // Encontra o menor slot que cabe o tamanho solicitado + cabecalho
    size_t needed = size + sizeof(PoolBlockHeader);
    for (int i = 0; i < pool->slot_count; i++) {
        PoolSlot* slot = &pool->slots[i];
        if (slot->block_size >= needed && slot->count > 0) {
            slot->count--;
            void* ptr = slot->free_list[slot->count];
            slot->free_list[slot->count] = NULL;

            // Store metadata at the beginning of the block
            // Armazena metadados no inicio do bloco
            PoolBlockHeader* hdr = (PoolBlockHeader*)ptr;
            hdr->pool = pool;
            hdr->slot = i;

            // Return pointer after the header
            // Retorna ponteiro depois do cabecalho
            return (uint8_t*)ptr + sizeof(PoolBlockHeader);
        }
    }

    // No slot available — return NULL (caller should fall back to block alloc)
    // Nenhum slot disponivel — retorna NULL (caller deve usar block alloc como fallback)
    return NULL;
}

void pool_free(PoolAllocator* pool, void* ptr) {
    if (!ptr || !pool) return;

    // Retrieve metadata from before the pointer
    // Recupera metadados de antes do ponteiro
    PoolBlockHeader* hdr = (PoolBlockHeader*)((uint8_t*)ptr - sizeof(PoolBlockHeader));
    if (hdr->pool != pool) return; // safety check

    int idx = hdr->slot;
    if (idx < 0 || idx >= pool->slot_count) return;

    PoolSlot* slot = &pool->slots[idx];
    if (slot->count >= slot->capacity) return; // shouldn't happen

    // Return to free list (pointer to the start of the block, before header)
    // Retorna para lista livre (ponteiro pro inicio do bloco, antes do cabecalho)
    slot->free_list[slot->count] = (void*)hdr;
    slot->count++;
}

void pool_destroy(PoolAllocator* pool) {
    if (!pool) return;
    for (int i = 0; i < pool->slot_count; i++) {
        free(pool->slots[i].data);
        free(pool->slots[i].free_list);
    }
    free(pool);
}

PoolStats pool_slot_stats(PoolAllocator* pool, int slot) {
    PoolStats stats = {0, 0, 0, 0};
    if (!pool || slot < 0 || slot >= pool->slot_count) return stats;
    PoolSlot* s = &pool->slots[slot];
    stats.block_size = s->block_size;
    stats.capacity   = s->capacity;
    stats.used       = s->capacity - s->count;
    stats.free       = s->count;
    return stats;
}

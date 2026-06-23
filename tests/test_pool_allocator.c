#include "pool_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { printf("  [PASS] %s\n", msg); tests_passed++; } \
    else { printf("  [FAIL] %s\n", msg); tests_failed++; } \
} while(0)

void test_pool_create_destroy() {
    printf("=== test_pool_create_destroy ===\n");
    PoolAllocator* pool = pool_create();
    CHECK(pool != NULL, "pool created");
    pool_destroy(pool);
    CHECK(1, "pool destroyed");
}

void test_pool_add_slot() {
    printf("=== test_pool_add_slot ===\n");
    PoolAllocator* pool = pool_create();
    int slot = pool_add_slot(pool, 64, 10);
    CHECK(slot == 0, "slot 0 added");
    PoolStats stats = pool_slot_stats(pool, 0);
    CHECK(stats.block_size == 64, "slot block_size=64");
    CHECK(stats.capacity == 10, "slot capacity=10");
    CHECK(stats.free == 10, "slot free=10");
    CHECK(stats.used == 0, "slot used=0");
    pool_destroy(pool);
}

void test_pool_alloc_free() {
    printf("=== test_pool_alloc_free ===\n");
    PoolAllocator* pool = pool_create();
    pool_add_slot(pool, 32, 5);
    pool_add_slot(pool, 64, 3);

    // Allocate from slot 0 (32 bytes)
    void* p1 = pool_alloc(pool, 4);
    CHECK(p1 != NULL, "pool_alloc 4 bytes returns non-NULL");

    void* p2 = pool_alloc(pool, 16);
    CHECK(p2 != NULL, "pool_alloc 16 bytes returns non-NULL");

    // Pointers should be distinct
    CHECK(p1 != p2, "consecutive allocs return distinct pointers");

    // Write and read to verify usable memory
    memset(p1, 0xAB, 4);
    memset(p2, 0xCD, 16);
    CHECK(((unsigned char*)p1)[0] == 0xAB, "write/read p1 OK");
    CHECK(((unsigned char*)p2)[0] == 0xCD, "write/read p2 OK");

    // Free and re-alloc (should reuse)
    pool_free(pool, p1);
    void* p3 = pool_alloc(pool, 4);
    CHECK(p3 == p1, "pool_free + pool_alloc reuses the block");

    pool_destroy(pool);
}

void test_pool_multiple_slots() {
    printf("=== test_pool_multiple_slots ===\n");
    PoolAllocator* pool = pool_create();
    pool_add_slot(pool, 32, 2);
    pool_add_slot(pool, 64, 2);
    pool_add_slot(pool, 128, 2);

    // Allocate from different slots based on size
    void* small = pool_alloc(pool, 4);    // fits in 32
    void* medium = pool_alloc(pool, 40);  // needs 64
    void* large = pool_alloc(pool, 100);  // needs 128

    CHECK(small != NULL, "small alloc from slot 0");
    CHECK(medium != NULL, "medium alloc from slot 1");
    CHECK(large != NULL, "large alloc from slot 2");

    // Different pointers from different slots
    CHECK(small != medium, "small != medium");
    CHECK(medium != large, "medium != large");

    PoolStats s0 = pool_slot_stats(pool, 0);
    PoolStats s1 = pool_slot_stats(pool, 1);
    PoolStats s2 = pool_slot_stats(pool, 2);
    CHECK(s0.used == 1, "slot0: 1 used");
    CHECK(s1.used == 1, "slot1: 1 used");
    CHECK(s2.used == 1, "slot2: 1 used");

    pool_free(pool, small);
    pool_free(pool, medium);
    pool_free(pool, large);

    s0 = pool_slot_stats(pool, 0);
    s1 = pool_slot_stats(pool, 1);
    s2 = pool_slot_stats(pool, 2);
    CHECK(s0.free == 2, "slot0: all free after free");
    CHECK(s1.free == 2, "slot1: all free after free");
    CHECK(s2.free == 2, "slot2: all free after free");

    pool_destroy(pool);
}

void test_pool_null_on_overflow() {
    printf("=== test_pool_null_on_overflow ===\n");
    PoolAllocator* pool = pool_create();
    // Need at least sizeof(PoolBlockHeader) + size bytes
    // Precisa de pelo menos sizeof(PoolBlockHeader) + size bytes
    pool_add_slot(pool, 64, 1);

    void* p1 = pool_alloc(pool, 4);
    CHECK(p1 != NULL, "first alloc succeeds");

    // Slot has only 1 block; second alloc should return NULL
    void* p2 = pool_alloc(pool, 4);
    CHECK(p2 == NULL, "second alloc returns NULL (slot exhausted)");

    pool_free(pool, p1);
    void* p3 = pool_alloc(pool, 4);
    CHECK(p3 != NULL, "after free, alloc succeeds again");

    pool_destroy(pool);
}

int main() {
    test_pool_create_destroy();
    test_pool_add_slot();
    test_pool_alloc_free();
    test_pool_multiple_slots();
    test_pool_null_on_overflow();

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

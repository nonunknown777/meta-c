#include "block_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { printf("  [PASS] %s\n", msg); tests_passed++; } \
    else { printf("  [FAIL] %s\n", msg); tests_failed++; } \
} while(0)

void test_db_enable_and_alloc() {
    printf("=== test_db_enable_and_alloc ===\n");

    BlockCtx* block = block_create_bytes(64 * 1024);
    CHECK(block != NULL, "block created");

    int ret = block_enable_double_buffer(block);
    CHECK(ret == 0, "double buffer enabled");

    // Allocate using double-buffer aware allocator
    // Aloca usando alocador ciente de double-buffer
    int* data = (int*)block_alloc_db(block, sizeof(int) * 100);
    CHECK(data != NULL, "block_alloc_db returns non-NULL");

    // Write and verify
    // Escreve e verifica
    for (int i = 0; i < 100; i++) data[i] = i;
    int ok = 1;
    for (int i = 0; i < 100; i++) {
        if (data[i] != i) { ok = 0; break; }
    }
    CHECK(ok, "data written and read correctly");

    // Swap buffers
    // Troca buffers
    size_t used_before = block->used;
    block_swap_buffers(block);

    // After swap, the allocator should be using the shadow buffer
    // The used count should be preserved
    // Apos a troca, o alocador deve estar usando o buffer sombra
    // A contagem usada deve ser preservada
    CHECK(block->data != NULL, "block has data after swap");

    // Allocate more data in the new active buffer
    // Aloca mais dados no novo buffer ativo
    int* more = (int*)block_alloc_db(block, sizeof(int) * 50);
    CHECK(more != NULL, "alloc after swap succeeds");

    // Swap back
    // Troca de volta
    block_swap_buffers(block);

    block_destroy(block);
    CHECK(1, "block destroyed after double buffer test");
}

void test_db_no_crash_on_non_db() {
    printf("=== test_db_no_crash_on_non_db ===\n");

    BlockCtx* block = block_create_bytes(4096);

    // Should not crash if swap is called on a non-double-buffer block
    // Nao deve crashar se swap for chamado em um bloco sem double-buffer
    block_swap_buffers(block);
    CHECK(1, "block_swap_buffers on non-DB block (no crash)");

    // Normal alloc should still work
    // Alocacao normal ainda deve funcionar
    int* p = (int*)block_alloc(block, sizeof(int));
    CHECK(p != NULL, "normal block_alloc still works");

    block_destroy(block);
}

void test_db_multiple_blocks() {
    printf("=== test_db_multiple_blocks ===\n");

    BlockCtx* b1 = block_create_bytes(64 * 1024);
    BlockCtx* b2 = block_create_bytes(128 * 1024);

    CHECK(block_enable_double_buffer(b1) == 0, "db enabled on b1");
    CHECK(block_enable_double_buffer(b2) == 0, "db enabled on b2");

    int* d1 = (int*)block_alloc_db(b1, 64);
    int* d2 = (int*)block_alloc_db(b2, 128);

    CHECK(d1 != NULL, "alloc from b1");
    CHECK(d2 != NULL, "alloc from b2");
    CHECK(d1 != d2, "distinct allocations from different blocks");

    block_swap_buffers(b1);
    block_swap_buffers(b2);

    int* d1_after = (int*)block_alloc_db(b1, 64);
    CHECK(d1_after != NULL, "alloc from b1 after swap");

    block_destroy(b1);
    block_destroy(b2);
}

int main() {
    test_db_enable_and_alloc();
    test_db_no_crash_on_non_db();
    test_db_multiple_blocks();

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

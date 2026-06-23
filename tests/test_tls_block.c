#include "block_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { printf("  [PASS] %s\n", msg); tests_passed++; } \
    else { printf("  [FAIL] %s\n", msg); tests_failed++; } \
} while(0)

#define THREAD_COUNT 4

typedef struct {
    int thread_id;
    int result;
} ThreadArg;

void* thread_worker(void* arg) {
    ThreadArg* ta = (ThreadArg*)arg;

    // Each thread creates its own block and sets it as TLS
    // Cada thread cria seu proprio bloco e define como TLS
    BlockCtx* block = block_create_bytes(1024 * 64);
    block_set_tls(block);

    // Verify get_tls returns the same block we set
    // Verifica se get_tls retorna o mesmo bloco que definimos
    BlockCtx* retrieved = block_get_tls();
    ta->result = (retrieved == block) ? 1 : 0;

    // Allocate from the TLS block — no lock needed
    // Aloca do bloco TLS — sem lock necessario
    int* data = (int*)block_alloc(block, sizeof(int) * 100);
    if (!data) {
        ta->result = 0;
    } else {
        // Verify we can write and read
        // Verifica se podemos escrever e ler
        for (int i = 0; i < 100; i++) {
            data[i] = ta->thread_id * 1000 + i;
        }
        int ok = 1;
        for (int i = 0; i < 100; i++) {
            if (data[i] != ta->thread_id * 1000 + i) {
                ok = 0;
                break;
            }
        }
        ta->result = ta->result && ok;
    }

    // Reset and destroy
    // Reseta e destroi
    block_reset(block);
    block_destroy(block);

    return NULL;
}

void test_tls_block_multithread() {
    printf("=== test_tls_block_multithread ===\n");

    pthread_t threads[THREAD_COUNT];
    ThreadArg args[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        args[i].thread_id = i;
        args[i].result = 0;
        pthread_create(&threads[i], NULL, thread_worker, &args[i]);
    }

    int all_ok = 1;
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
        if (!args[i].result) all_ok = 0;
    }

    CHECK(all_ok, "all threads: TLS block set/get OK, allocations OK");
}

void test_tls_get_set() {
    printf("=== test_tls_get_set ===\n");

    CHECK(block_get_tls() == NULL, "initial TLS is NULL");

    BlockCtx* b1 = block_create_bytes(1024);
    BlockCtx* b2 = block_create_bytes(2048);

    block_set_tls(b1);
    CHECK(block_get_tls() == b1, "TLS set to b1");

    block_set_tls(b2);
    CHECK(block_get_tls() == b2, "TLS set to b2");

    block_set_tls(NULL);
    CHECK(block_get_tls() == NULL, "TLS set to NULL");

    block_destroy(b1);
    block_destroy(b2);
}

int main() {
    test_tls_get_set();
    test_tls_block_multithread();

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

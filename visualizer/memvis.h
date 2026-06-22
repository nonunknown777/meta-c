#ifndef BRICK_MEMVIS_H
#define BRICK_MEMVIS_H

#include "../runtime/block_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int refresh_ms;
    int follow_fork;
} MemVisConfig;

#define MEMVIS_DEFAULT_CONFIG { .refresh_ms = 500, .follow_fork = 0 }

// Start the TUI — reads blocks from the runtime registry (embedded mode)
// Inicia a TUI — le blocos do registro runtime (modo embutido)
// Falls back to demo blocks if no registered blocks exist
// Usa blocos de demonstracao se nenhum bloco registrado existir
void memvis_run(MemVisConfig config);

// Attach to a running process by reading /tmp/brick-mem-<pid>.bin
// Anexa a um processo em execucao lendo /tmp/brick-mem-<pid>.bin
void memvis_attach(int pid, MemVisConfig config);

// Read block snapshot from a shared memory file (for external viewer)
// Le um snapshot de blocos de um arquivo de memoria compartilhada (para visualizador externo)
// Returns number of blocks read, or 0 on error
// Retorna numero de blocos lidos, ou 0 em erro
int memvis_read_shm(int pid, BlockInfo* blocks, int max_blocks);

#ifdef __cplusplus
}
#endif

#endif // BRICK_MEMVIS_H
     // BRICK_MEMVIS_H

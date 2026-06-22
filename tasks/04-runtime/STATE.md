# Estado Atual - Runtime
# Current State - Runtime

Sessão: 2026-06-22
Session: 2026-06-22

Progresso: 100%
Progress: 100%

Última ação: Correção de warnings -Wall -Werror, mmap para grandes alocações, mais testes, fix print(char)
Last action: Fixed -Wall -Werror warnings, mmap for large allocations, more tests, fix print(char)

## Realizado (nesta sessão)
## Completed (this session)

### 1. Correção: `-Wall -Werror` com `BRICK_TRACK_BLOCKS` desligado
### 1. Fix: `-Wall -Werror` without `BRICK_TRACK_BLOCKS`

`block_shm_export()` no-op macro usava `(-1)` que gerava `-Werror=unused-value`
quando chamado como statement. Mudado para `((void)0)`.

`block_shm_export()` no-op macro used `(-1)` which triggered `-Werror=unused-value`
when called as a statement. Changed to `((void)0)`.

Auto-export em `block_alloc_aligned` e `block_reset` agora é condicional via
`#ifdef BRICK_TRACK_BLOCKS`, eliminando chamadas desnecessárias quando o
registro está desligado.

Auto-export in `block_alloc_aligned` and `block_reset` is now conditional via
`#ifdef BRICK_TRACK_BLOCKS`, eliminating unnecessary calls when the registry
is disabled.

### 2. Performance: mmap para grandes alocações (>= 64KB)
### 2. Performance: mmap for large allocations (>= 64KB)

`block_create_bytes()` agora usa `mmap()` com `MAP_POPULATE` para alocações
>= 64KB, permitindo huge pages e melhor desempenho. Alocações menores que
64KB continuam usando malloc. `block_destroy()` usa `munmap()` para blocos
mmap'd.

`block_create_bytes()` now uses `mmap()` with `MAP_POPULATE` for allocations
>= 64KB, enabling huge pages and better performance. Allocations smaller than
64KB continue using malloc. `block_destroy()` uses `munmap()` for mmap'd blocks.

### 3. Testes: +7 novos testes de runtime (14 total)
### 3. Tests: +7 new runtime tests (14 total)

- `test_create_bytes` — criação com tamanho exato
- `test_zero_size_alloc` — alocação de tamanho zero
- `test_large_alignment` — alinhamento explícito 16/32/64 bytes
- `test_peak_used` — verifica rastreamento de pico
- `test_allocation_count` — verifica contagem de alocações (não reseta)
- `test_stress` — 10000 alocações pequenas em loop
- `test_freeze_thaw_blocked` — freeze/thaw API não crasha

- `test_create_bytes` — creation with exact size
- `test_zero_size_alloc` — zero size allocation
- `test_large_alignment` — explicit 16/32/64 byte alignment
- `test_peak_used` — verify peak usage tracking
- `test_allocation_count` — verify allocation count (no reset)
- `test_stress` — 10000 small allocations in a loop
- `test_freeze_thaw_blocked` — freeze/thaw API doesn't crash

### 4. Correção: `print('a')` gerava `io_print_u8` em vez de `io_print_char`
### 4. Fix: `print('a')` generated `io_print_u8` instead of `io_print_char`

`gen_print_single()` em codegen.cpp verificava `tn == "char"` depois de
`normalize_type_name()` que mapeia `char` → `u8`. Agora verifica o tipo
original `t == "char"` antes da normalização.

`gen_print_single()` in codegen.cpp checked `tn == "char"` after
`normalize_type_name()` which maps `char` → `u8`. Now checks the original
type `t == "char"` before normalization.

### 5. PROGRESS.md atualizado
### 5. PROGRESS.md updated

Todas as tarefas agora marcadas como concluídas.

All tasks now marked as completed.

## Pendências
## Pending

- Nenhuma
- None

# Próximo Passo - Runtime
# Next Step - Runtime

## Feature: Tipos Explícitos de Largura Fixa — Melhorias de Runtime
## Feature: Explicit Fixed-Width Types — Runtime Improvements

### Realizado (task 10)
### Completed (task 10)

### 1. io.h — Type-specific print functions

Adicionadas funções `io_print_u8/u16/u32/u64`, `io_print_i8/i16/i32/i64`,
`io_print_f32/f64`, `io_print_usize/isize` em `runtime/io.h`.

Cada função usa o format specifier PRI exato do `<inttypes.h>`, eliminando
widening implícito (ex: antes `u8` virava `(long long)` pra chamar
`io_print_int`).

### 2. `BrickString.len` mudou de `int64_t` para `size_t`

Consistência com o resto do runtime que já usa `size_t` (capacidade do
bloco, índices de alocação). `io_print_string` também mudou o parâmetro
`len` para `size_t`.

### 3. Bump allocator — alinhamento adaptativo

`block_alloc()` agora calcula alinhamento ótimo baseado no tamanho:
- size 1 → align 1 (u8/bool/char — zero waste)
- size 2 → align 2 (u16 — zero waste)
- size 4 → align 4 (u32/f32 — zero waste)
- size 8+ → align 8 (default, cache-line friendly)

`block_alloc_aligned(ctx, size, alignment)` continua respeitando alinhamento
explícito quando chamado diretamente.

### 4. codegen.cpp atualizado

- `gen_print_single` faz dispatch direto pra `io_print_u8`, `io_print_i32` etc
- `gen_printf_call` usa specifiers `PRIu8/PRId32/PRIu64` etc
- BrickString literal gera `.len=(size_t)N`
- Inclui `<inttypes.h>` no cabeçalho C gerado

### Testes

79/79 testes passando.

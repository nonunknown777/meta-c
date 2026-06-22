# Estado Atual - Codegen
# Current State - Codegen

Sessão: 2026-06-22
Session: 2026-06-22

Progresso: 100%
Progress: 100%

Última ação: Implementação de tipos explícitos de largura fixa (u8/i8..u64/i64, f32/f64, usize/isize)
Last action: Implementation of explicit fixed-width types (u8/i8..u64/i64, f32/f64, usize/isize)

## Realizado
## Completed

- Type checker: detecta `using IO;` → seta `using_io = true`
- Type checker: valida print() args (int/float/String/bool/char), retorna void
- Type checker: erro se print() sem `using IO;`
- Codegen: `#include "io.h"` no cabeçalho (em vez de gerar BrickString)
- Codegen: `gen_string_type()` removido — BrickString vem do io.h
- Codegen: print() 0 args → `io_print_newline()`
- Codegen: print() 1 arg sem placeholders → `io_print_X(arg)`
- Codegen: print() com formato `{N}` → `io_printf()` com specifiers
- Codegen: `block_register(name, "name")` emitido após cada `block_create_bytes()`
- Codegen: `block_shm_export()` emitido ao final de `__brick_init()`
- Type checker: `is_type_known()` atualizado com tipos de largura fixa
- Type checker: `can_assign()` com regras de widening/narrowing (Signed↔Unsigned, Int↔Float)
- Type checker: `promote_types()` para expressões mistas (i8+u16→i32, i32+u32→i64, etc.)
- Type checker: validação de overflow em literais com/sem sufixo (ex: `u8 x = 300` → erro)
- Type checker: inferência contextual de literais sem sufixo (ex: `u8 x = 42` → 42 cabe em u8)
- Codegen: `map_type()` mapeia `i32`/`int`→`int32_t`, `f64`/`double`→`double`, etc.
- Codegen: cast explícito em literais com sufixo (`(uint8_t)42`)
- Codegen: `gen_print_single/gen_printf_call` suportam novos tipos
- Testes: 79/79 passando + compilação gcc -Wall -Werror
- Testes novos: fixed_width_types, literal_suffix, literal_overflow, type_promotion

- Type checker: detects `using IO;` → sets `using_io = true`
- Type checker: validates print() args (int/float/String/bool/char), returns void
- Type checker: error if print() without `using IO;`
- Codegen: `#include "io.h"` in header (instead of generating BrickString)
- Codegen: `gen_string_type()` removed — BrickString comes from io.h
- Codegen: print() 0 args → `io_print_newline()`
- Codegen: print() 1 arg without placeholders → `io_print_X(arg)`
- Codegen: print() with format `{N}` → `io_printf()` with specifiers
- Codegen: `block_register(name, "name")` emitted after each `block_create_bytes()`
- Codegen: `block_shm_export()` emitted at the end of `__brick_init()`
- Type checker: `is_type_known()` updated with fixed-width types
- Type checker: `can_assign()` with widening/narrowing rules (Signed↔Unsigned, Int↔Float)
- Type checker: `promote_types()` for mixed expressions (i8+u16→i32, i32+u32→i64, etc.)
- Type checker: overflow validation on literals with/without suffix (e.g. `u8 x = 300` → error)
- Type checker: contextual inference of unsuffixed literals (e.g. `u8 x = 42` → 42 fits in u8)
- Codegen: `map_type()` maps `i32`/`int`→`int32_t`, `f64`/`double`→`double`, etc.
- Codegen: explicit cast on suffixed literals (`(uint8_t)42`)
- Codegen: `gen_print_single/gen_printf_call` support new types
- Tests: 79/79 passing + gcc -Wall -Werror compilation
- New tests: fixed_width_types, literal_suffix, literal_overflow, type_promotion

## Pendências
## Pending

- Nenhuma — feature completa
- None — feature complete

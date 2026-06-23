# Task 09 - Debugger - NEXT

## Status: ✅ Todas as pendências pós-otimizações resolvidas

### PoolAllocator
- [x] Pretty-printer para `PoolAllocator` (mostrar slots, blocks, free, used)
- [x] `info blocks` / `block <name>` mostrar info do pool (`__pool_<name>`)

### TLS Blocks
- [x] `info blocks` listar `_tls_current_block` se != NULL (via `gdb.lookup_global_symbol()`)
- [x] `block-watch` funcionar com todos os alloc variants (`block_alloc`, `block_alloc_aligned`, `block_alloc_db`)

### Double-Buffer
- [x] Função `block_has_double_buffer()` no runtime
- [x] `block <name>` mostrar status do double-buffer

### Inline Hints
- [x] Comando `brick debug` recomendar `scons debug=1`

## Pendências futuras (baixa prioridade)
- [ ] Testar debugging com inline hints ativos vs desativados
- [ ] Testar pretty-printers com GDB em um programa Brick real
- [ ] VS Code Memory View integration (webview)
- [ ] Adicionar testes GDB automatizados (ex: gdb -batch -x test.gdb)
- [ ] Trocar `exec()` por `import` no .gdbinit
- [ ] Adicionar suporte a `$_pool_names` convenience variable (similar a `$_block_names`)
- [ ] Melhorar `block-watch` para usar Python breakpoints com conditions (em vez de `$rdi`/`$x0`)

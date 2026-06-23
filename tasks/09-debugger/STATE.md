# Task 09 - Debugger - STATE

## Status: ✅ COMPLETO (atualizado em 2026-06-23)

## Implementado — Core
- `debugger/.gdbinit` — carrega pretty-printers e comandos automaticamente
- `debugger/gdb_pretty_printers.py` (268 linhas) — 4 printers:
  - `BlockCtxPrinter`: formata BlockCtx com barra de uso
  - `BrickStringPrinter`: mostra string + length
  - `PoolAllocatorPrinter`: mostra slots, block_size, capacity, free, used (com barras)
  - `BlockAllocPrinter`: mostra `@block+offset` para ponteiros em blocos
- `debugger/gdb_commands.py` (469 linhas) — 6 comandos:
  - `blocks-list` — lista nomes de blocos (usado pelo VS Code memory webview)
  - `info blocks` / `ib` — tabela formatada com barras Unicode, flag TLS, coluna `-p` opcional (pool)
  - `block <name>` — detalhes de um bloco (capacity, used, free, peak, allocs, double-buffer, pool slots)
  - `block-watch <name>` / `bw` — breakpoint em block_alloc / block_alloc_aligned / block_alloc_db com detecção de arquitetura (x86-64 $rdi, ARM64 $x0)
  - `blocks-list` — lista nomes de blocos (um por linha, para DAP)
  - `brick debug` / `bd` — conselhos de debug, recomenda `scons debug=1`
- `#line` directives no codegen — debug mapping funcional
- GDB mostra código-fonte Brick original

## Implementado — PoolAllocator
- Pretty-printer `PoolAllocatorPrinter` em gdb_pretty_printers.py
- `info blocks -p` / `ibp` mostra coluna de pool (yes/--)
- `block <name>` mostra detalhes de cada slot do pool (block_size, capacity, used, free)
- Alias `ibp` para `info blocks -p`

## Implementado — TLS Blocks
- `_tls_current_block` detectado via `gdb.lookup_global_symbol()`
- Mostrado em `info blocks` com flag `T` ao lado do nome
- Aparece como `TLS` no nome da coluna

## Implementado — Double-Buffer
- Função `block_has_double_buffer(BlockCtx*)` adicionada ao runtime (block_memory.h + block_memory.c)
- `block <name>` mostra `DoubleBuf:  active` se o bloco tiver double-buffer ativado

## Implementado — Inline Hints
- Comando `brick debug` recomenda `scons debug=1` para desativar inline hints

## Arquivos Alterados
- `debugger/gdb_pretty_printers.py` — +PoolAllocatorPrinter +registro
- `debugger/gdb_commands.py` — rewrite completo: +pool scan, +TLS scan, +pool info, +double-buffer, +arch-aware block-watch, +brick debug
- `debugger/.gdbinit` — novos aliases (ibp, bw, bd)
- `runtime/block_memory.h` — declaração `block_has_double_buffer()`
- `runtime/block_memory.c` — implementação `block_has_double_buffer()`

## Observações
- `block-watch` usa arch-aware register names (x86: $rdi, ARM: $x0) — mais portável
- `exec()` ainda usado no .gdbinit (em vez de `import`) — pode ser refatorado
- Sem testes GDB automatizados — testes manuais via .gdbinit
- Comandos bilíngues PT/EN mantidos

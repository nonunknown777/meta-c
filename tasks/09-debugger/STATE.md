# Estado Atual
# Current State

Sessão: 4 (GDB 17 compat + dynamic discovery fix)
Session: 4 (GDB 17 compat + dynamic discovery fix)

Progresso: 100%
Progress: 100%

Próximo passo: Pendente — ver NEXT.md
Next step: Pending — see NEXT.md

Última ação: Correções de compatibilidade GDB 17 + descoberta dinâmica
Last action: GDB 17 compatibility fixes + dynamic discovery

Pendências: NOVA FEATURE — Tipos Explícitos de Largura Fixa (ver NEXT.md)
Pending: NEW FEATURE — Explicit Fixed-Width Types (see NEXT.md)

## Features verificadas (GDB 17 batch)
## Features verified (GDB 17 batch)

- [x] `$_block_names` auto-populado via hook-stop: `"strs,_current_block,global"`
- [x] `blocks-list` command: lista nomes dos blocos dinâmicos
- [x] `info variables -t BlockCtx` (GDB 17+): filtra por tipo
- [x] `info blocks`: mostra 3 blocos reais com tamanhos corretos (global 8MB, strs 1MB, _current_block 8MB)
- [x] 3 abordagens no webview: `$_block_names` (hover) → `info variables -t BlockCtx` (REPL) → `blocks-list` (REPL)

- [x] `$_block_names` auto-populated via hook-stop: `"strs,_current_block,global"`
- [x] `blocks-list` command: lists dynamic block names
- [x] `info variables -t BlockCtx` (GDB 17+): filters by type
- [x] `info blocks`: shows 3 real blocks with correct sizes (global 8MB, strs 1MB, _current_block 8MB)
- [x] 3 approaches in webview: `$_block_names` (hover) → `info variables -t BlockCtx` (REPL) → `blocks-list` (REPL)

## Corrigido nesta sessão
## Fixed this session

### GDB 17 API breaking changes
### GDB 17 API breaking changes

- `objfile.global_block()` não existe mais → substituído por `frame.block().global_block` (propriedade)
- `objfile.static_block()` não existe mais → removido, global_block cobre ambos
- `info variables BlockCtx` não filtra por tipo → usar `info variables -t BlockCtx` com flag `-t`
- `python` dentro de `define` no .gdbinit: indentação precisa estar na coluna 0

- `objfile.global_block()` no longer exists → replaced by `frame.block().global_block` (property)
- `objfile.static_block()` no longer exists → removed, global_block covers both
- `info variables BlockCtx` does not filter by type → use `info variables -t BlockCtx` with `-t` flag
- `python` inside `define` in .gdbinit: indentation must be at column 0

### Melhorias de robustez
### Robustness improvements

- `gdb.events.stop.connect()` adicionado como handler secundário (GDB 7.9+)
- .gdbinit hook-stop corrigido (IndentationError → funcionando)
- 3 abordagens em cascata no webview (fallback method)
- debugador `debugger/` adicionado ao `sys.path` no .gdbinit para import confiável

- `gdb.events.stop.connect()` added as secondary handler (GDB 7.9+)
- .gdbinit hook-stop fixed (IndentationError → working)
- 3 cascade approaches in webview (fallback method)
- `debugger/` directory added to `sys.path` in .gdbinit for reliable import

### Debug de binário
### Binary debug

- Descoberto que usuário executava `meus-testes/main` (17KB, SEM debug symbols)
- Binário correto: `build/main` (37KB, COM debug_info + #line directives)

- Discovered that user was running `meus-testes/main` (17KB, NO debug symbols)
- Correct binary: `build/main` (37KB, WITH debug_info + #line directives)

## Implementado (sessão anterior: dynamic discovery + realtime)
## Implemented (previous session: dynamic discovery + realtime)

- Descoberta dinâmica de blocos (sem lista hardcoded)
- Comando GDB `blocks-list`
- Real-time: webview atualiza a cada pause/step/breakpoint
- Frontend sem demo data

- Dynamic block discovery (no hardcoded list)
- GDB command `blocks-list`
- Real-time: webview updates on every pause/step/breakpoint
- Frontend without demo data

## Implementado (sessões anteriores)
## Implemented (previous sessions)

- #line directives no codegen
- GDB pretty-printers (BlockCtx, BrickString, BlockAlloc)
- GDB custom commands (info blocks, block, block-watch)
- .gdbinit com auto-load
- VS Code debug config (launch.json + tasks.json)
- VS Code webview panel (Brick Memory)
- Block Data Provider para debug

- #line directives in codegen
- GDB pretty-printers (BlockCtx, BrickString, BlockAlloc)
- GDB custom commands (info blocks, block, block-watch)
- .gdbinit with auto-load
- VS Code debug config (launch.json + tasks.json)
- VS Code webview panel (Brick Memory)
- Block Data Provider for debugging

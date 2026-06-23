# Task 08 - VS Code Extension - STATE

## Status: 🔶 PARCIAL (atualizado pela Task 10)

## Implementado
- Estrutura de extensão VS Code completa (package.json, tsconfig)
- Syntax highlighting (TextMate grammar em `syntaxes/brick.tmLanguage.json`)
- 19 snippets em `snippets/brick.code-snippets`
- LSP server (`server/src/server.ts`, 868 linhas)
- Language service com scanner próprio (`server/src/languageService.ts`, 915 linhas)
- Compiler service que chama `brick --lsp` (`server/src/compilerService.ts`)
- Debug webview (`src/memoryWebview.ts`, 382 linhas) com barras de uso, alertas >75%/90%
- Comandos: `brick.initWorkspace`, `brick.debugProgram`
- Testes do scanner LSP (`server/test/scanner.test.ts`, 408 linhas, 19 suites)

## ✅ O que foi implementado (Task 08 - VSCoder)

### Pool allocator — IMPLEMENTADO
- Runtime ganhou `runtime/pool_allocator.h/.c` com `PoolAllocator`, `pool_create`, `pool_add_slot`, `pool_alloc`, `pool_free`, `pool_destroy`
- Codegen gera `PoolAllocator* __pool_<nome>` para cada block
- `PoolAllocator` adicionado como tipo built-in no `languageService.ts` (BUILTIN_TYPES)
- `PoolAllocator` adicionado ao `typeKeywordSet` no `server.ts`
- `PoolAllocator` adicionado à grammar (`brick.tmLanguage.json`) como `storage.type.brc`
- Memory webview: detecta e exibe `__pool_<nome>` com barras de uso ao lado dos BlockCtx

### TLS blocks — IMPLEMENTADO
- Runtime: `block_set_tls(BlockCtx*)`, `block_get_tls()`, `block_alloc_tls(size)`
- Codegen: `__brick_init()` chama `block_set_tls(_current_block)` automaticamente
- Grammar: sem mudança (não adicionamos keyword `thread`)
- LSP: `block_set_tls`, `block_get_tls`, `block_alloc_tls` adicionados ao autocomplete

### Double-buffer — COMPLETO
- Runtime: `block_enable_double_buffer`, `block_swap_buffers`, `block_alloc_db`
- LSP: funções adicionadas ao autocomplete com docs
- Memory webview: detecta blocos double-buffered via `_db_ctx_table`/`_db_ext_table` no GDB
- Exibe badge "DB BUF 0/1" indicando buffer ativo ao lado do nome do bloco

### Outras funções runtime — IMPLEMENTADO
- `pool_create`, `pool_add_slot`, `pool_alloc`, `pool_free`, `pool_destroy`
- `block_enable_double_buffer`, `block_swap_buffers`, `block_alloc_db`
- Todas adicionadas ao autocomplete com assinaturas e docs

## Arquivos
- `vscode-ext/` — Diretório da extensão

## Observações
- Scanner TypeScript (`languageService.ts`) duplica lógica do lexer C++ — precisa manter sync manual
- Memory webview poderia mostrar `__pool_*` e blocos double-buffered
- `block_set_tls`/`block_alloc_tls` precisam ser adicionados ao completion provider

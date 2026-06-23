# Task 08 - VS Code Extension - NEXT

## ✅ Concluído nesta sessão
- [x] `PoolAllocator` adicionado como tipo built-in no LSP (`languageService.ts`, `server.ts`, grammar)
- [x] TODAS as funções runtime adicionadas ao autocomplete (pool_*, block_set_tls, block_get_tls, block_alloc_tls, block_enable_double_buffer, block_swap_buffers, block_alloc_db)
- [x] Docs para PoolAllocator e funções runtime adicionados em `languageService.ts`
- [x] Memory webview: `__pool_<nome>` exibido ao lado dos BlockCtx

## Pendências
- [x] Memory webview: indicar double-buffer (ativo vs shadow) via `_db_ctx_table`/`_db_ext_table`
- [ ] Compilar e testar a extensão localmente com `npm run compile`
- [ ] Verificar testes do scanner LSP (`server/test/scanner.test.ts`)
- [ ] Publicar extensão no VS Code Marketplace

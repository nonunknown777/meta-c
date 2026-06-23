# Task 10 - Tester / Optimizer / Doc - NEXT

## Pendências

### 0. Sistema de Macros (estado atual)
- ✅ 10/10 integration tests passando
- ✅ Macros funcionam em nível top-level e dentro de funções
- ✅ `emit { }` com conteúdo real (não mais vazio)
- ✅ `$interpolation` em nomes de variáveis dentro de declarações em macros
- ❌ `$` em nomes de função dentro de `emit {}` não funciona — `func_decl()` só aceita `IDENTIFIER`
- ❌ Build variables (`msg = "text"` dentro de `build {}`) não interpolam automaticamente em `emit {}`
- ❌ `docs/MACROS.md` e `docs/MACROS.pt-BR.md` — criar documentação com exemplos de jogo

### 1. Documentação (parcial)
- ~~docs/*.md com visão geral para leigos~~ ✅ revisados/corrigidos + OPTIMIZATIONS.md reescrito com deep-dive
- ~~index.html~~ ✅ seção de otimizações adicionada
- ~~README.md~~ ✅ tabela resumo das 7 otimizações
- ✅ MACROS.md e MACROS.pt-BR.md criados com exemplos game-oriented
- Verificar wiki/ files (Language-Reference.md, Memory-Blocks.md, Architecture.md) — podem estar desatualizados
- Verificar se LANGUAGE.pt-BR.md precisa de atualização correspondente
- Adicionar comentários inline no código onde faltam
- Documentar runtime (block_memory.c, io.c)

### 2. Otimizações (implementadas ✅)
- ~~Inline hints~~ ✅ codegen
- ~~SIMD alignment~~ ✅ float/f64 fields
- ~~Constant folding~~ ✅ codegen
- ~~Pool allocator~~ ✅ runtime + codegen
- ~~Thread-local blocks~~ ✅ runtime + `block_set_tls()` automático
- ~~Pauseless hot reload (double-buffer)~~ ✅ runtime
- ~~PGO build profile~~ ✅ SConstruct
- Inline asm em hot paths
- Compiler profiling (tempo de compilação)
- Generated code profiling (runtime performance)
- String interning

### 3. Testes adicionais
- Testes de C interop com mais bibliotecas
- Testes de type checker (mais casos edge)
- Testes de erro (narrowing, type mismatch)
- Benchmark suite (compile time + runtime)
- Testes de macros: error test .brc files (undefined macro, wrong arg count, $ outside macro, I/O in build, recursive)

### 4. Coordenação
- Verificar estado real tasks 08 e 09
- CI/CD setup (GitHub Actions)
- Atualizar shared-context.md se necessário

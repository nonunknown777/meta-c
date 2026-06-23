# Task 10 - Tester / Optimizer - NEXT

## Pendências

### 1. Documentação (parcial)
- ~~docs/*.md com visão geral para leigos~~ ✅ revisados/corrigidos + OPTIMIZATIONS.md reescrito com deep-dive
- ~~index.html~~ ✅ seção de otimizações adicionada
- ~~README.md~~ ✅ tabela resumo das 7 otimizações
- Verificar wiki/ files (Language-Reference.md, Memory-Blocks.md, Architecture.md) — podem estar desatualizados
- Verificar se LANGUAGE.pt-BR.md precisa de atualização correspondente
- Adicionar comentários inline no código onde faltam
- Documentar runtime (block_memory.c, io.c)

### 2. Otimizações (todas implementadas ✅)
- ~~Inline hints for gcc~~ ✅ codegen
- ~~SIMD alignment~~ ✅ float/f64 fields
- ~~Constant folding~~ ✅ codegen
- ~~Pool allocator~~ ✅ runtime + integração automática no codegen
- ~~Thread-local blocks~~ ✅ runtime + `block_set_tls()` automático em `__brick_init`
- ~~Pauseless hot reload (double-buffer)~~ ✅ runtime
- ~~PGO build profile~~ ✅ SConstruct
- Inline asm em hot paths (próximo passo)
- Compiler profiling (tempo de compilação)
- Generated code profiling (runtime performance)
- String interning

### 3. Testes adicionais
- Testes de C interop com mais bibliotecas
- Testes de type checker (mais casos edge)
- Testes de erro (narrowing, type mismatch)
- Benchmark suite (compile time + runtime)
- Teste end-to-end: compilar .brc com pool allocator, verificar no C gerado

### 4. Coordenação
- Verificar estado real tasks 08 e 09
- CI/CD setup (GitHub Actions)
- Atualizar shared-context.md se necessário

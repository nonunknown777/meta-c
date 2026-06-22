# Próximo Passo - Tester/Optimizer
# Next Step - Tester/Optimizer

## Sessão 8 — COMPLETA ✅

- [x] Migração completa .mc → .brc
- [x] Arquivos renomeados (.mc → .brc, mc-run.sh → brc-run.sh)
- [x] src/main.cpp atualizado (CLI mostra .brc)
- [x] Testes atualizados (test_lexer, test_codegen, test_integration)
- [x] Scripts atualizados (benchmarks, brc-run.sh)
- [x] VS Code Extension atualizada (package.json, extension.ts, server.ts, tmLanguage.json)
- [x] docs/, wiki/, READMEs, tasks/* atualizados
- [x] Build: sucesso, sem warnings
- [x] Testes unitários: 151/151 passando
- [x] Testes de integração: 5/5 passando
- [x] Benchmarks: compile 6ms, alloc 0.002s/1M (21× malloc)

## Próximas ações sugeridas (prioridade)

| Prio | Task | Ação |
|------|------|------|
| 1 | **08-vscoder** | Debug webview: trocar demo data por GDB real (3 abordagens cascata) + publicar no Marketplace |
| 2 | **09-debugger** | Pretty-printers para novos tipos C (uint8_t, int32_t, float, double, size_t, ptrdiff_t) |
| 3 | **02-parser** | Validação de conflito de nomes (`IO`/`print` vs `using IO;`) |
| 4 | **11-libs** | Próximas bibliotecas: input (teclado/mouse/gamepad), audio, file, net, math |
| 5 | **Documentação** | `docs/*.md` para leigos (visão geral da linguagem, tutoriais) |
| 6 | **Otimização** | Profiling do compilador (hot spots no parsing/codegen), SIMD em runtime se aplicável |

## Priorities (English)

| Prio | Task | Action |
|------|------|--------|
| 1 | **08-vscoder** | Debug webview: replace demo data with real GDB (3 cascade approaches) + publish to Marketplace |
| 2 | **09-debugger** | Pretty-printers for new C types (uint8_t, int32_t, float, double, size_t, ptrdiff_t) |
| 3 | **02-parser** | Name conflict validation (`IO`/`print` vs `using IO;`) |
| 4 | **11-libs** | Next libraries: input (keyboard/mouse/gamepad), audio, file, net, math |
| 5 | **Documentation** | `docs/*.md` for beginners (language overview, tutorials) |
| 6 | **Optimization** | Compiler profiling (hot spots in parsing/codegen), SIMD in runtime if applicable |

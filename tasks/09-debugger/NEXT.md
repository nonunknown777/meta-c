# Próximo Passo - Debugger
# Next Step - Debugger

## Feature: Tipos Explícitos de Largura Fixa
## Feature: Explicit Fixed-Width Types

### Pretty-printers

Atualizar os pretty-printers GDB (Python) para reconhecer e exibir
corretamente os novos tipos C:
- `uint8_t` / `uint16_t` / `uint32_t` / `uint64_t`
- `int8_t` / `int16_t` / `int32_t` / `int64_t`
- `float` / `double`
- `size_t` / `ptrdiff_t`

A maioria já tem suporte nativo no GDB, mas verificar se os
pretty-printers customizados do projeto (BlockCtx, BrickString)
não são afetados.

### Webview Memory View

Se o webview mostra valores de variáveis, atualizar display types
para incluir os novos tipos primitivos.

## Pendências anteriores
## Previous pending

- Hot reload integration com debugger (parar em swap)
- Testes automatizados de debug (GDB batch mode no CI)
- Suporte a watchpoints em blocos específicos
- Cache nos pretty-printers para performance com muitos blocos
- Se o REPL evaluate dos approaches não funcionar em algumas versões do cppdbg, melhorar fallback
- Verificar se `_current_block` mostra 8MB (deve ser ponteiro reatribuído, não bloco próprio)

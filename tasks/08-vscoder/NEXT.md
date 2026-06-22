# Próximo Passo - VSCoder
# Next Step - VSCoder

## ✅ Concluído — Tipos Explícitos de Largura Fixa
## ✅ Completed — Explicit Fixed-Width Types

### Syntax Highlighting ✅

Adicionados ao grammar `syntaxes/brick.tmLanguage.json`:
`u8|u16|u32|u64|i8|i16|i32|i64|f32|f64|usize|isize|byte`

### Language Service ✅

Atualizado `languageService.ts`:
- KEYWORDS + KEYWORD_DOCS + BUILTIN_TYPES com os novos tipos
- Regex de `isAfterType` atualizada

### Server Completions ✅

Atualizado `server.ts`:
- `typeKeywordSet` com novos tipos
- `keywordCompletions` com entradas para cada tipo
- Context-aware após `private`/`public`

## Pendências anteriores
## Previous pending

- Debug webview memory view com dados reais do GDB (vs dados demo atuais)
- Parser principal (task 02) precisa atualizar para if sem parênteses e @
- Adicionar testes para o LSP server
- Publicar extensão no Marketplace VS Code

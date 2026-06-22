# Brick Design Document

> Linguagem OOP que compila para C. Performance máxima, memória em blocos, hot reload.

## Projeto
- **Nome:** Brick
- **Extensão:** `.brc`
- **CLI:** `brick input.brc -o output.c`
- **Sintaxe:** GDScript-inspired com chaves `{ }`, tipos explícitos
- **Paradigma:** Struct-OOP (structs com métodos)
- **Compilador:** C++20, gera C puro
- **Runtime:** C (block allocator + hot reload)
- **Build:** SCons
- **Plataforma:** Linux (Arch)

## Linguagem
- Visibilidade: `public` (padrão) | `private`
- Struct: `struct Nome { }`
- Herança: `struct Filha extends Mae { }`
- Interface: `interface Nome { fn metodo(); }`
- Construtor: `fn StructName() { }` (mesmo nome da struct)
- Sem `this`, sem shadow de nomes
- Entry: `fn main()`
- Erro: `error("msg")` panic

## Memória
- Tudo em blocos gerenciados pelo usuário (zero stack)
- Bloco anônimo interno do compilador pra params/retorno
- Declaração: `block nome = N KB/MB/GB`
- Default: `global` (obrigatório declarar no main)
- Escopo: `block nome: { }`
- Inline: `int x = 5 @nome`
- Liberação: `nome.reset()` (bump allocator, sem free individual)
- Bloco cheio: erro runtime (panic)
- Ref cruzada entre blocos: ✅

## Packages
- `package NOME` declara, `using NOME` importa
- Hierárquico: `package SPRITES.EFFECTS`
- Visível por padrão, `private` restringe

## Compilação
```
.brc → Lexer → Parser + Package Resolver → Codegen → .c (com #line) → gcc -g → binário
```

## Debugging
- `#line` directives no C gerado mapeiam .brc → .c
- GDB mostra código-fonte Brick original
- GDB pretty-printers para BlockCtx e String
- Comandos GDB custom: `info blocks`, `block <nome>`
- VS Code webview com blocos visuais durante debug

## Ferramentas
- **Hot Reload:** dlopen + inotify, swap de .so por package
- **Visualizador:** TUI ncurses (blocos em tempo real)
- **VS Code:** Syntax highlight + snippets + LSP + Memory Webview (debug)
- **Testes:** Unitários + integração (.brc → binário)
- **Debugger:** GDB + VS Code integrados com suporte a blocos
- **Tester/Optimizer:** Task sênior que testa, otimiza e documenta tudo

## Tasks
```
01-lexer      Tokenizer .brc → tokens
02-parser     AST + Package Resolution
03-codegen    Type check + geração C (com #line p/ debug)
04-runtime    Block memory allocator (C)
05-hotreload  dlopen + inotify
06-visualizer TUI ncurses
07-builder    SCons build
08-vscoder    VS Code extension (highlight + LSP + debug webview)
09-debugger   GDB pretty-printers + #line directives + VS Code Memory View
10-tester     Test runner + optimizer + documentation + coordinator
```

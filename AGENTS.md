# Brick

## Projeto
## Project
Linguagem de programação que compila para C. Foco em performance máxima,
gerenciamento explícito de memória por blocos, e hot reload nativo.
A programming language that compiles to C. Focus on maximum performance,
explicit block-based memory management, and native hot reload.

## Stack
- Compilador: C++20
- Runtime: C (ABI externa pra hot reload)
- Build: SCons
- Visualizador: ncurses (TUI)
- Hot Reload: dlopen + inotify
- Compiler: C++20
- Runtime: C (external ABI for hot reload)
- Build: SCons
- Visualizer: ncurses (TUI)
- Hot Reload: dlopen + inotify

## Estrutura
## Structure
```
src/           → compilador (C++20) / compiler (C++20)
runtime/       → block memory allocator + hot reload (C)
visualizer/    → TUI ncurses (C++)
debugger/      → GDB pretty-printers + .gdbinit (Python)
tasks/         → 10 tasks, cada uma com AGENTS.md + estado / each with AGENTS.md + state
examples/      → código .brc de exemplo / example .brc code
tests/         → testes unitários / unit tests
vscode-ext/    → extensão VS Code (highlight + LSP + debug webview) / VS Code extension
```

## Tasks
Cada task em tasks/ tem seu próprio AGENTS.md com instruções específicas.
Sempre leia STATE.md e NEXT.md ao iniciar uma sessão.
Each task in tasks/ has its own AGENTS.md with specific instructions.
Always read STATE.md and NEXT.md when starting a session.

```
01-lexer      Tokenizer .brc -> tokens
02-parser     AST + Package Resolution
03-codegen    Type check + geracao C (com #line p/ debug)
04-runtime    Block memory allocator (C)
05-hotreload  dlopen + inotify
06-visualizer TUI ncurses
07-builder    SCons build
08-vscoder    VS Code extension (highlight + LSP + debug webview)
09-debugger   GDB pretty-printers + #line directives + VS Code Memory View
10-tester     Test runner + optimizer + documentation + coordinator
11-libs       Official libraries (window, input, audio, file, net, math)
```

## Convenções
## Conventions
- C++20, snake_case pra funções, PascalCase pra structs
- Runtime headers SEMPRE com extern "C"
- Toda feature com teste em tests/
- Código C gerado deve ser legível
- Sem `this`, sem shadow de nomes
- error() pra panic
- C++20, snake_case for functions, PascalCase for structs
- Runtime headers ALWAYS with extern "C"
- Every feature with a test in tests/
- Generated C code must be readable
- No `this`, no name shadowing
- error() for panic

## Spec completa
## Full Spec
Veja shared-context.md para a especificação completa da linguagem.
See shared-context.md for the complete language specification.

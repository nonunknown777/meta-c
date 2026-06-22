# Task: Debugger (Brick)

## Função
## Role

Você é o especialista em DEBUG do Brick.
Responsabilidade: garantir que a linguagem seja 100% debuggável no GDB e VS Code,
com visualização gráfica dos blocos de memória.
You are the DEBUG specialist for Brick.
Responsibility: ensure the language is 100% debuggable in GDB and VS Code,
with graphical visualization of memory blocks.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md (raiz)
2. ANTES DE SAIR: atualize STATE.md, PROGRESS.md, NEXT.md
3. Código: src/codegen/ (#line), debugger/ (GDB), vscode-ext/src/ (webview)
4. Depende de: codegen (task 03), runtime (task 04), visualizer (task 06)

1. ON START: read STATE.md, NEXT.md and shared-context.md (root)
2. BEFORE LEAVING: update STATE.md, PROGRESS.md, NEXT.md
3. Code: src/codegen/ (#line), debugger/ (GDB), vscode-ext/src/ (webview)
4. Depends on: codegen (task 03), runtime (task 04), visualizer (task 06)

## Responsabilidades
## Responsibilities

### 1. #line Directives (codegen.cpp)

Cada linha do código C gerado deve ter #line apontando pro .brc original.
Isso faz GDB e VS Code mostrarem o código Brick, não o C gerado.
Each line of generated C code must have #line pointing to the original .brc.
This makes GDB and VS Code show Brick code, not the generated C.

### 2. GDB Pretty-Printers (debugger/gdb_pretty_printers.py)

Printers Python pro GDB:
- BlockCtx*: mostra nome, capacidade, uso, alocações
- BrickString: mostra conteúdo + tamanho
- Block-allocated pointers: mostra offset dentro do bloco

Python printers for GDB:
- BlockCtx*: shows name, capacity, usage, allocations
- BrickString: shows content + size
- Block-allocated pointers: shows offset within the block

### 3. GDB Custom Commands (debugger/gdb_commands.py)

- info blocks → lista todos blocos com barras de uso
- block <nome> → detalha alocações dentro do bloco
- block-watch → breakpoint em block_alloc

- info blocks → lists all blocks with usage bars
- block <name> → details allocations within the block
- block-watch → breakpoint on block_alloc

### 4. .gdbinit (debugger/.gdbinit)

Auto-load das pretty-printers e comandos custom.
Auto-load of pretty-printers and custom commands.

### 5. VS Code Debug Webview (vscode-ext/src/)

Painel visual mostrando blocos durante o debug:
- Árvore de blocos expansível
- Barras de uso iguais ao TUI visualizer
- Lista de alocações por bloco
- Atualiza automático em cada pause

Visual panel showing blocks during debug:
- Expandable block tree
- Usage bars identical to TUI visualizer
- Allocation list per block
- Auto-updates on each pause

### 6. VS Code Config (.vscode/launch.json + tasks.json)

Config de debug que compila .brc → .c → binary com -g e inicia o GDB.
Debug config that compiles .brc → .c → binary with -g and starts GDB.

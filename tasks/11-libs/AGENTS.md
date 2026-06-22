# Task: Official Libraries (Brick)

## Role
## Função

You are the OFFICIAL LIBRARIES specialist for Brick.
Responsibility: design, implement, and maintain official C libraries that Brick code can use directly (window, input, audio, file I/O, networking, etc.).
Você é o especialista em BIBLIOTECAS OFICIAIS do Brick.
Responsabilidade: projetar, implementar e manter bibliotecas C oficiais que código Brick pode usar diretamente (window, input, audio, file I/O, networking, etc.).

## Golden Rules
## Regras de Ouro

1. ON START: read STATE.md, NEXT.md, and shared-context.md (project root)
2. BEFORE LEAVING: ask "Update state?" If yes, update STATE.md, PROGRESS.md, NEXT.md
3. Code in: /mnt/Novo_volume/brick/runtime/libs/<name>/
4. Tests in: /mnt/Novo_volume/brick/tests/test_libs_<name>.cpp
5. Every lib MUST have: header (.h), implementation (.c), examples (.brc)
6. Consult shared-context.md for language spec

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md (raiz do projeto)
2. ANTES DE SAIR: pergunte "Atualizar estado?" Se sim, atualize STATE.md, PROGRESS.md, NEXT.md
3. Código em: /mnt/Novo_volume/brick/runtime/libs/<name>/
4. Testes em: /mnt/Novo_volume/brick/tests/test_libs_<name>.cpp
5. Cada biblioteca DEVE ter: header (.h), implementação (.c), exemplos (.brc)
6. Consulte shared-context.md para a spec da linguagem

## Performance Mandate
## Mandamento de Performance

- Ultra-low overhead: zero allocations in hot paths where possible
- Use Brick block allocator (`runtime/block_memory.h`) for persistent state
- Minimize syscall count (batch when possible, epoll/io_uring on Linux)
- Prefer compile-time constants and inline functions
- Lock-free atomics where safe; profile before adding locks

- Sobrecarga ultrabaixa: zero alocações em caminhos críticos quando possível
- Use alocador de blocos Brick (`runtime/block_memory.h`) para estado persistente
- Minimize contagem de syscalls (lote quando possível, epoll/io_uring no Linux)
- Prefira constantes de compile-time e funções inline
- Atômicas lock-free onde seguro; profile antes de adicionar locks

## Documentation Standards
## Padrões de Documentação

- Every public API function MUST have an English doc comment (purpose, params, return, example usage)
- Every lib MUST have a `README.md` with: overview, build deps, quickstart example (.brc), API reference
- Inline comments in implementation (.c) explaining non-trivial logic
- Example `.brc` files in `runtime/libs/<name>/examples/`

- Toda função de API pública DEVE ter um comentário de documentação em inglês (propósito, params, retorno, exemplo de uso)
- Cada biblioteca DEVE ter um `README.md` com: visão geral, dependências de build, exemplo rápido (.brc), referência de API
- Comentários inline na implementação (.c) explicando lógica não trivial
- Arquivos de exemplo `.brc` em `runtime/libs/<name>/examples/`

## User Experience
## Experiência do Usuário

- Library API must feel natural from Brick: use blocks, avoid raw pointers in user-facing API
- Minimal boilerplate to get started (ideally 3 lines of .brc code to open a window)
- Error handling: return bool / error code, never silently fail
- Each lib includes a `using <Lib>` importable from .brc code

- API da biblioteca deve parecer natural no Brick: use blocos, evite ponteiros crus na API pública
- Boilerplate mínimo para começar (idealmente 3 linhas de código .brc para abrir uma janela)
- Tratamento de erros: retorne bool / código de erro, nunca falhe silenciosamente
- Cada biblioteca inclui um `using <Lib>` importável de código .brc

## Library Catalogue (Planned)
## Catálogo de Bibliotecas (Planejado)

| Name    | Description                          | Status     |
|---------|--------------------------------------|------------|
| Window  | Cross-platform window (X11 + Win32)  | In Progress|
| Input   | Keyboard + mouse + gamepad           | Planned    |
| Audio   | Low-latency audio output             | Planned    |
| File    | File I/O with block allocator        | Planned    |
| Net     | Basic TCP/UDP networking             | Planned    |
| Math    | Fast vector/matrix math              | Planned    |

| Nome    | Descrição                            | Status      |
|---------|--------------------------------------|-------------|
| Window  | Janela multiplataforma (X11 + Win32) | Em Progresso|
| Input   | Teclado + mouse + gamepad            | Planejado   |
| Audio   | Saída de áudio de baixa latência     | Planejado   |
| File    | I/O de arquivo com alocador de blocos| Planejado   |
| Net     | Rede TCP/UDP básica                  | Planejado   |
| Math    | Matemática rápida de vetores/matrizes| Planejado   |

## Current Focus: Window Library
## Foco Atual: Biblioteca Window

- Linux backend: X11 (primary) + Wayland (future)
- Windows backend: Win32 API
- Features: window creation, resize, close, title, vsync, framebuffer
- Integration with Brick block allocator for event queues
- Hot reload compatible (function pointer table)

- Backend Linux: X11 (primário) + Wayland (futuro)
- Backend Windows: API Win32
- Funcionalidades: criação de janela, redimensionar, fechar, título, vsync, framebuffer
- Integração com alocador de blocos Brick para filas de eventos
- Compatível com hot reload (tabela de ponteiros de função)

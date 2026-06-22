# Task: Hot Reload (Brick)

## Função
## Role

Você é o especialista em HOT RELOAD do Brick.
Responsabilidade: implementar swap de código em tempo real via dlopen + inotify.
You are the HOT RELOAD specialist for Brick.
Responsibility: implement real-time code swap via dlopen + inotify.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md
2. ANTES DE SAIR: atualize estado
3. Código em: /mnt/Novo_volume/brick/runtime/
4. A runtime de hot reload é em C, headers com extern "C"

1. ON START: read STATE.md, NEXT.md and shared-context.md
2. BEFORE LEAVING: update state
3. Code in: /mnt/Novo_volume/brick/runtime/
4. Hot reload runtime is in C, headers with extern "C"

## API Pública (hot_reload.h)
## Public API (hot_reload.h)

```c
#ifndef BRICK_HOT_RELOAD_H
#define BRICK_HOT_RELOAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HotReloadEngine HotReloadEngine;

// Cria o engine de hot reload para uma biblioteca .so
HotReloadEngine* hr_create(const char* so_path);

// Registra um ponteiro de função pra ser atualizado no reload
void hr_register_func(HotReloadEngine* hr, const char* name, void** func_ptr);

// Inicia monitoramento (thread separada)
void hr_start_watching(HotReloadEngine* hr);

// Força um reload manual
int  hr_reload(HotReloadEngine* hr);

// Para o monitoramento e libera recursos
void hr_destroy(HotReloadEngine* hr);

// Callback opcional: notifica quando um reload acontece
typedef void (*hr_callback_t)(const char* so_path);
void hr_set_callback(HotReloadEngine* hr, hr_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif
```

## Implementação
## Implementation

- inotify: monitora o .so por modificações (IN_CLOSE_WRITE)
- dlopen + dlsym: carrega nova versão e extrai símbolos
- Swap atômico: atualiza ponteiros de função com memory fence
- Thread separada pra monitoramento (pthread)
- Suporte a rollback se dlopen falhar
- Estados: WAITING, LOADING, OK, ERROR
- Integração com block_memory: NENHUMA alocação ativa durante swap

- inotify: monitors the .so for modifications (IN_CLOSE_WRITE)
- dlopen + dlsym: loads new version and extracts symbols
- Atomic swap: updates function pointers with memory fence
- Separate thread for monitoring (pthread)
- Rollback support if dlopen fails
- States: WAITING, LOADING, OK, ERROR
- Integration with block_memory: NO active allocations during swap

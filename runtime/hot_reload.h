#ifndef BRICK_HOT_RELOAD_H
#define BRICK_HOT_RELOAD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HR_WAITING,
    HR_LOADING,
    HR_OK,
    HR_ERROR,
} HotReloadState;

typedef struct HotReloadEngine HotReloadEngine;

// Function pointer type for reload callbacks
// Tipo do ponteiro de funcao para callbacks de reload
typedef void (*hr_callback_t)(const char* so_path);

// Create a hot reload engine for a .so library
// Cria um motor de hot reload para uma biblioteca .so
HotReloadEngine* hr_create(const char* so_path);

// Load initial symbols from the .so
// Carrega simbolos iniciais do .so
int hr_load_initial(HotReloadEngine* hr);

// Register a function pointer for hot swapping
// Registra um ponteiro de funcao para troca a quente
// name: symbol name in the .so
// name: nome do simbolo no .so
// func_ptr: address of a void* that will be updated on reload
// func_ptr: endereco de um void* que sera atualizado no reload
int hr_register_func(HotReloadEngine* hr, const char* name, void** func_ptr);

// Start watching the .so file for modifications (inotify)
// Inicia monitoramento do arquivo .so por modificacoes (inotify)
// Creates a separate monitoring thread
// Cria uma thread de monitoramento separada
int hr_start_watching(HotReloadEngine* hr);

// Force a reload immediately
// Forca um reload imediatamente
int hr_reload(HotReloadEngine* hr);

// Get current state
// Obtem estado atual
HotReloadState hr_state(HotReloadEngine* hr);

// Set a callback that fires after every reload attempt
// Define um callback que executa apos cada tentativa de reload
void hr_set_callback(HotReloadEngine* hr, hr_callback_t cb);

// Stop monitoring and cleanup
// Para monitoramento e limpa
void hr_destroy(HotReloadEngine* hr);

#ifdef __cplusplus
}
#endif

#endif // BRICK_HOT_RELOAD_H
     // BRICK_HOT_RELOAD_H

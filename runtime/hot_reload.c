#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L
#include "hot_reload.h"
#include "block_memory.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <stdatomic.h>

#define MAX_SYMBOLS 256
#define EVENT_BUF_LEN 4096

typedef struct {
    char  name[128];
    void** func_ptr;
} SymbolEntry;

struct HotReloadEngine {
    char          so_path[1024];
    char          so_basename[256];
    char          so_path_tmp[1024];  // path + ".new" for atomic swap
                                       // path + ".new" para troca atomica
    void*         handle_current;
    void*         handle_new;
    SymbolEntry   symbols[MAX_SYMBOLS];
    int           symbol_count;
    atomic_int    state;
    pthread_t     watch_thread;
    int           inotify_fd;
    int           watch_fd;
    volatile int  running;
    hr_callback_t callback;
    pthread_mutex_t mutex;
};

static void error(const char* msg) {
    fprintf(stderr, "Brick hot reload error: %s\n", msg);
    exit(1);
}

HotReloadEngine* hr_create(const char* so_path) {
    HotReloadEngine* hr = (HotReloadEngine*)calloc(1, sizeof(HotReloadEngine));
    if (!hr) error("out of memory");

    strncpy(hr->so_path, so_path, sizeof(hr->so_path) - 1);

    // Extract basename
    // Extrai o nome base
    const char* slash = strrchr(so_path, '/');
    strncpy(hr->so_basename, slash ? slash + 1 : so_path, sizeof(hr->so_basename) - 1);

    snprintf(hr->so_path_tmp, sizeof(hr->so_path_tmp), "%s.new", so_path);

    hr->state = HR_WAITING;
    hr->running = 0;
    hr->callback = NULL;
    hr->inotify_fd = -1;
    hr->watch_fd = -1;
    pthread_mutex_init(&hr->mutex, NULL);

    return hr;
}

int hr_load_initial(HotReloadEngine* hr) {
    pthread_mutex_lock(&hr->mutex);

    hr->handle_current = dlopen(hr->so_path, RTLD_NOW | RTLD_LOCAL);
    if (!hr->handle_current) {
        fprintf(stderr, "hr: dlopen failed: %s\n", dlerror());
        hr->state = HR_ERROR;
        pthread_mutex_unlock(&hr->mutex);
        return -1;
    }

    hr->state = HR_OK;

    // Resolve registered symbols
    // Resolve simbolos registrados
    for (int i = 0; i < hr->symbol_count; i++) {
        void* sym = dlsym(hr->handle_current, hr->symbols[i].name);
        if (sym) {
            *(hr->symbols[i].func_ptr) = sym;
        } else {
            fprintf(stderr, "hr: symbol '%s' not found\n", hr->symbols[i].name);
        }
    }

    pthread_mutex_unlock(&hr->mutex);
    return 0;
}

int hr_register_func(HotReloadEngine* hr, const char* name, void** func_ptr) {
    if (hr->symbol_count >= MAX_SYMBOLS) return -1;

    strncpy(hr->symbols[hr->symbol_count].name, name,
            sizeof(hr->symbols[hr->symbol_count].name) - 1);
    hr->symbols[hr->symbol_count].func_ptr = func_ptr;
    hr->symbol_count++;
    return 0;
}

int hr_reload(HotReloadEngine* hr) {
    pthread_mutex_lock(&hr->mutex);
    hr->state = HR_LOADING;

    // Freeze block allocations during swap
    // Congela alocacoes de bloco durante a troca
    block_freeze();

    // Copy the .so to a temp path so dlopen gets a fresh load
    // Copia o .so para um caminho temporario para dlopen obter carga fresca
    // (dlopen caches by path; a copy bypasses the cache)
    // (dlopen armazena em cache por caminho; uma copia contorna o cache)
    char tmp_path[1088];
    snprintf(tmp_path, sizeof(tmp_path), "%s.%d", hr->so_path, (int)time(NULL));

    FILE* src = fopen(hr->so_path, "rb");
    void* new_handle = NULL;
    if (src) {
        FILE* dst = fopen(tmp_path, "wb");
        if (dst) {
            char buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
                fwrite(buf, 1, n, dst);
            fclose(dst);

            new_handle = dlopen(tmp_path, RTLD_NOW | RTLD_LOCAL);
            unlink(tmp_path);
        }
        fclose(src);
    }

    if (!new_handle) {
        const char* err = dlerror();
        fprintf(stderr, "hr: reload failed: %s\n", err ? err : "cannot open .so file");
        // Rollback: keep old handle, function pointers untouched
        // Rollback: mantem handle antigo, ponteiros de funcao intocados
        hr->state = hr->handle_current ? HR_OK : HR_ERROR;
        block_thaw();
        if (hr->callback) hr->callback(hr->so_path);
        pthread_mutex_unlock(&hr->mutex);
        return -1;
    }

    // Swap function pointers atomically
    // Troca ponteiros de funcao atomicamente
    for (int i = 0; i < hr->symbol_count; i++) {
        void* sym = dlsym(new_handle, hr->symbols[i].name);
        if (sym) {
            __atomic_store(hr->symbols[i].func_ptr, &sym, __ATOMIC_SEQ_CST);
        } else {
            fprintf(stderr, "hr: symbol '%s' not found in new .so\n",
                    hr->symbols[i].name);
        }
    }

    // Close old handle
    // Fecha handle antigo
    if (hr->handle_current) {
        dlclose(hr->handle_current);
    }
    hr->handle_current = new_handle;
    hr->state = HR_OK;

    block_thaw();
    if (hr->callback) hr->callback(hr->so_path);

    pthread_mutex_unlock(&hr->mutex);
    return 0;
}

// Watch thread function
// Funcao da thread de monitoramento
static void* watch_thread_fn(void* arg) {
    HotReloadEngine* hr = (HotReloadEngine*)arg;

    char buf[EVENT_BUF_LEN] __attribute__((aligned(8)));

    while (hr->running) {
        ssize_t len = read(hr->inotify_fd, buf, sizeof(buf));
        if (len <= 0) continue;

        size_t i = 0;
        while (i < (size_t)len) {
            struct inotify_event* event = (struct inotify_event*)&buf[i];

            if ((event->mask & IN_CLOSE_WRITE) &&
                event->len > 0 &&
                strcmp(event->name, hr->so_basename) == 0) {
                // Small delay to let file writes complete
                // Pequeno atraso para permitir que escritas no arquivo terminem
                struct timespec ts = {0, 50000000};
                nanosleep(&ts, NULL);
                hr_reload(hr);
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

    return NULL;
}

int hr_start_watching(HotReloadEngine* hr) {
    hr->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (hr->inotify_fd < 0) {
        perror("inotify_init");
        return -1;
    }

    // Watch directory of the .so file
    // Monitora diretorio do arquivo .so
    char dir_path[1024];
    size_t path_len = strlen(hr->so_path);
    if (path_len >= sizeof(dir_path)) path_len = sizeof(dir_path) - 1;
    memcpy(dir_path, hr->so_path, path_len);
    dir_path[path_len] = '\0';
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) *last_slash = '\0';
    else strcpy(dir_path, ".");

    hr->watch_fd = inotify_add_watch(hr->inotify_fd, dir_path, IN_CLOSE_WRITE);
    if (hr->watch_fd < 0) {
        perror("inotify_add_watch");
        return -1;
    }

    hr->running = 1;
    if (pthread_create(&hr->watch_thread, NULL, watch_thread_fn, hr) != 0) {
        perror("pthread_create");
        hr->running = 0;
        return -1;
    }

    return 0;
}

HotReloadState hr_state(HotReloadEngine* hr) {
    return (HotReloadState)atomic_load(&hr->state);
}

void hr_set_callback(HotReloadEngine* hr, hr_callback_t cb) {
    hr->callback = cb;
}

void hr_destroy(HotReloadEngine* hr) {
    if (hr->running) {
        hr->running = 0;
        pthread_join(hr->watch_thread, NULL);
    }

    if (hr->inotify_fd >= 0) close(hr->inotify_fd);
    if (hr->handle_current) dlclose(hr->handle_current);
    if (hr->handle_new) dlclose(hr->handle_new);

    pthread_mutex_destroy(&hr->mutex);
    free(hr);
}

#ifndef BRICK_WINDOW_HR_H
#define BRICK_WINDOW_HR_H

#include "window.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Hot-reloadable function pointer table ─── */

typedef struct {
    void* create;
    void* destroy;
    void* poll_events;
    void* next_event;
    void* swap_buffers;
    void* should_close;
    void* set_title;
    void* get_size;
    void* set_fullscreen;
    void* native_handle;
} MetaWindowFuncTable;

/* Global table — populated by meta_window_hr_init().
 * On hot reload the entries are atomically swapped. */
extern MetaWindowFuncTable meta_window_table;

/* ─── Type-safe inline wrappers ─── */

static inline MetaWindow* meta_win_create(
    BlockCtx* block, const char* title, int width, int height, uint32_t flags
) {
    MetaWindow* (*fn)(BlockCtx*, const char*, int, int, uint32_t) =
        (MetaWindow* (*)(BlockCtx*, const char*, int, int, uint32_t))meta_window_table.create;
    return fn(block, title, width, height, flags);
}

static inline void meta_win_destroy(MetaWindow* w) {
    void (*fn)(MetaWindow*) = (void (*)(MetaWindow*))meta_window_table.destroy;
    fn(w);
}

static inline int meta_win_poll_events(MetaWindow* w) {
    int (*fn)(MetaWindow*) = (int (*)(MetaWindow*))meta_window_table.poll_events;
    return fn(w);
}

static inline int meta_win_next_event(MetaWindow* w, MetaEvent* out) {
    int (*fn)(MetaWindow*, MetaEvent*) =
        (int (*)(MetaWindow*, MetaEvent*))meta_window_table.next_event;
    return fn(w, out);
}

static inline void meta_win_swap_buffers(MetaWindow* w) {
    void (*fn)(MetaWindow*) = (void (*)(MetaWindow*))meta_window_table.swap_buffers;
    fn(w);
}

static inline int meta_win_should_close(MetaWindow* w) {
    int (*fn)(MetaWindow*) = (int (*)(MetaWindow*))meta_window_table.should_close;
    return fn(w);
}

static inline void meta_win_set_title(MetaWindow* w, const char* title) {
    void (*fn)(MetaWindow*, const char*) =
        (void (*)(MetaWindow*, const char*))meta_window_table.set_title;
    fn(w, title);
}

static inline void meta_win_get_size(MetaWindow* w, int* out_w, int* out_h) {
    void (*fn)(MetaWindow*, int*, int*) =
        (void (*)(MetaWindow*, int*, int*))meta_window_table.get_size;
    fn(w, out_w, out_h);
}

static inline void meta_win_set_fullscreen(MetaWindow* w, int fullscreen) {
    void (*fn)(MetaWindow*, int) =
        (void (*)(MetaWindow*, int))meta_window_table.set_fullscreen;
    fn(w, fullscreen);
}

static inline void* meta_win_native_handle(MetaWindow* w) {
    void* (*fn)(MetaWindow*) = (void* (*)(MetaWindow*))meta_window_table.native_handle;
    return fn(w);
}

/* ─── Forward declaration of HotReloadEngine ─── */

struct HotReloadEngine;

/* ─── Initialization ─── */

/* Register all window symbols with an existing HotReloadEngine.
 * Call hr_load_initial() afterwards, or just use this which does it.
 * Returns 0 on success, -1 on failure. */
int meta_window_hr_init(struct HotReloadEngine* hr);

/* Convenience: creates a HotReloadEngine, registers all symbols,
 * loads initial, and starts watching the .so at so_path.
 * Returns the engine on success, NULL on failure.
 * The caller owns the engine and must call hr_destroy() on shutdown. */
struct HotReloadEngine* meta_window_hr_start(const char* so_path);

#ifdef __cplusplus
}
#endif

#endif /* BRICK_WINDOW_HR_H */

#ifndef BRICK_WINDOW_H
#define BRICK_WINDOW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration — full type in block_memory.h */
typedef struct BlockCtx BlockCtx;

/* ─── Window creation flags ─── */

typedef enum {
    META_WINDOW_NONE       = 0,
    META_WINDOW_RESIZABLE  = 1 << 0,
    META_WINDOW_FULLSCREEN = 1 << 1,
    META_WINDOW_HIDDEN     = 1 << 2,
    META_WINDOW_BORDERLESS = 1 << 3,
    META_WINDOW_VSYNC      = 1 << 4,
} MetaWindowFlags;

/* ─── Event types ─── */

typedef enum {
    META_EVENT_NONE        = 0,
    META_EVENT_CLOSE       = 1,
    META_EVENT_RESIZE      = 2,
    META_EVENT_KEY_DOWN    = 3,
    META_EVENT_KEY_UP      = 4,
    META_EVENT_MOUSE_MOVE  = 5,
    META_EVENT_MOUSE_BTN   = 6,
    META_EVENT_MOUSE_WHEEL = 7,
} MetaEventType;

typedef struct {
    MetaEventType type;
    int64_t       timestamp_ms;
    union {
        struct { int width, height; }          resize;
        struct { int keycode, mods; }          key;
        struct { double x, y; }                mouse_move;
        struct { int button, action, mods; }   mouse_btn;
        struct { double delta; }               mouse_wheel;
    } data;
} MetaEvent;

/* ─── Window handle ─── */

typedef struct MetaWindowImpl MetaWindow;

/**
 * Creates a window with the given title, dimensions, and flags.
 *
 * The MetaWindow struct is allocated from the given BlockCtx.
 * Call meta_window_destroy() to close the native window and release
 * OS resources, then reset or destroy the block to reclaim the memory.
 *
 * Returns NULL on failure.
 *
 * Example (Brick):
 *   using Window
 *   block win = 1MB
 *   Window w = Window.create("Hello", 800, 600, Window.RESIZABLE | Window.VSYNC) @win
 *
 * Example (C):
 *   BlockCtx* block = block_create_bytes(65536);
 *   MetaWindow* w = meta_window_create(block, "Hello", 800, 600, META_WINDOW_VSYNC);
 */
MetaWindow* meta_window_create(
    BlockCtx* block,
    const char* title,
    int width, int height,
    uint32_t flags
);

/**
 * Destroys the native window and releases OS resources (X11/Win32).
 * Does NOT free the MetaWindow struct — that memory belongs to the
 * BlockCtx passed at creation. Call block_reset() or block_destroy()
 * to reclaim the memory.
 */
void meta_window_destroy(MetaWindow* w);

/**
 * Polls all pending events. Returns the number of events enqueued.
 * Call meta_window_next_event() to retrieve them one by one.
 */
int meta_window_poll_events(MetaWindow* w);

/**
 * Retrieves the next pending event. Returns 0 if no more events.
 * Must be called in a loop after meta_window_poll_events().
 */
int meta_window_next_event(MetaWindow* w, MetaEvent* out);

/**
 * Swaps the front and back buffers. Call after rendering each frame.
 */
void meta_window_swap_buffers(MetaWindow* w);

/**
 * Returns non-zero if the close button has been pressed.
 */
int meta_window_should_close(MetaWindow* w);

/**
 * Sets the window title at runtime.
 */
void meta_window_set_title(MetaWindow* w, const char* title);

/**
 * Returns the current window dimensions.
 */
void meta_window_get_size(MetaWindow* w, int* out_width, int* out_height);

/**
 * Toggles fullscreen mode.
 */
void meta_window_set_fullscreen(MetaWindow* w, int fullscreen);

/**
 * Returns the native window handle (Window on X11, HWND on Win32).
 * Useful for advanced interop. Use sparingly.
 */
void* meta_window_native_handle(MetaWindow* w);

#ifdef __cplusplus
}
#endif

#endif /* BRICK_WINDOW_H */

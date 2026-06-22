#ifndef BRICK_WINDOW_INTERNAL_H
#define BRICK_WINDOW_INTERNAL_H

#include "window.h"
#include "../../block_memory.h"

/* ─── Event queue ring buffer ─── */
#define META_WINDOW_EVENT_CAPACITY 256

typedef struct {
    MetaEvent  events[META_WINDOW_EVENT_CAPACITY];
    int        head;
    int        tail;
    int        count;
} EventQueue;

static inline void event_queue_init(EventQueue* q) {
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}

static inline int event_queue_push(EventQueue* q, const MetaEvent* e) {
    if (q->count >= META_WINDOW_EVENT_CAPACITY) return 0;
    q->events[q->tail] = *e;
    q->tail = (q->tail + 1) % META_WINDOW_EVENT_CAPACITY;
    q->count++;
    return 1;
}

static inline int event_queue_pop(EventQueue* q, MetaEvent* out) {
    if (q->count == 0) return 0;
    *out = q->events[q->head];
    q->head = (q->head + 1) % META_WINDOW_EVENT_CAPACITY;
    q->count--;
    return 1;
}

/* ─── Platform-specific state ─── */

struct MetaWindowImpl {
    /* Shared */
    EventQueue   event_queue;
    char         title[256];
    int          width;
    int          height;
    uint32_t     flags;
    int          should_close;

    /* Block allocator context for this window's data */
    BlockCtx*    block;

#if defined(__linux__)
    /* X11 state */
    void*        display;   /* Display* */
    unsigned long window;   /* Window */
    unsigned long gc;       /* GC */
#elif defined(_WIN32)
    /* Win32 state */
    void*        hinstance; /* HINSTANCE */
    void*        hwnd;      /* HWND */
    void*        hdc;       /* HDC */
#endif
};

#endif /* BRICK_WINDOW_INTERNAL_H */

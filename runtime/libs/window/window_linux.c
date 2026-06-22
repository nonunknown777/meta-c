/**
 * window_linux.c — X11 backend for Brick Window library
 *
 * Uses Xlib directly (no XCB) for lower overhead and simpler sync.
 * The event queue is allocated from the Brick block allocator.
 */

#include "window_internal.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Reuse io.h's printf-style logging — or we add a simple log macro. */
#include "../../io.h"

/* ─── Key mapping helpers ─── */

static int x11_keycode_to_meta(KeySym ks) {
    switch (ks) {
        case XK_Escape:    return 256;
        case XK_Return:    return 257;
        case XK_Tab:       return 258;
        case XK_Left:      return 263;
        case XK_Right:     return 262;
        case XK_Up:        return 265;
        case XK_Down:      return 264;
        case XK_Shift_L:
        case XK_Shift_R:   return 340;
        case XK_Control_L:
        case XK_Control_R: return 341;
        case XK_Alt_L:
        case XK_Alt_R:     return 342;
        case XK_space:     return 32;
        default:
            if (ks >= XK_a && ks <= XK_z) return (int)ks - XK_a + 97;
            if (ks >= XK_0 && ks <= XK_9) return (int)ks;
            return 0;
    }
}

/* ─── Creation ─── */

MetaWindow* meta_window_create(
    BlockCtx* block, const char* title, int width, int height, uint32_t flags
) {
    if (!block) return NULL;

    MetaWindow* w = (MetaWindow*)block_alloc(block, sizeof(MetaWindow));
    if (!w) return NULL;

    event_queue_init(&w->event_queue);
    w->width  = width;
    w->height = height;
    w->flags  = flags;
    w->block  = block;

    if (title) {
        size_t len = strlen(title);
        if (len >= sizeof(w->title)) len = sizeof(w->title) - 1;
        memcpy(w->title, title, len);
        w->title[len] = '\0';
    } else {
        memcpy(w->title, "Brick", 7);
    }

    /* Open X display */
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) {
        io_print_string("FATAL: Cannot open X display", 28);
        return NULL;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    /* Window attributes */
    XSetWindowAttributes attrs;
    attrs.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask | FocusChangeMask;

    Window xwin = XCreateWindow(
        dpy, root,
        0, 0, width, height, 0,
        CopyFromParent, InputOutput,
        CopyFromParent,
        CWEventMask, &attrs
    );

    /* Set title via WM hints */
    XStoreName(dpy, xwin, w->title);
    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, xwin, &wm_delete, 1);

    /* Resizability */
    if (!(flags & META_WINDOW_RESIZABLE)) {
        XSizeHints hints;
        hints.flags = PMinSize | PMaxSize;
        hints.min_width  = width;
        hints.max_width  = width;
        hints.min_height = height;
        hints.max_height = height;
        XSetWMNormalHints(dpy, xwin, &hints);
    }

    /* Fullscreen */
    if (flags & META_WINDOW_FULLSCREEN) {
        Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom full = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
        XChangeProperty(dpy, xwin, wm_state, XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&full, 1);
    }

    GC gc = XCreateGC(dpy, xwin, 0, NULL);

    /* Sync before storing */
    XMapWindow(dpy, xwin);
    XFlush(dpy);

    w->display = (void*)dpy;
    w->window  = (unsigned long)xwin;
    w->gc      = (unsigned long)gc;

    return w;
}

/* ─── Destruction ─── */

void meta_window_destroy(MetaWindow* w) {
    if (!w) return;
    Display* dpy = (Display*)w->display;
    if (dpy) {
        XFreeGC(dpy, (GC)w->gc);
        XDestroyWindow(dpy, (Window)w->window);
        XCloseDisplay(dpy);
    }
    w->display = NULL;
}

/* ─── Event polling ─── */

int meta_window_poll_events(MetaWindow* w) {
    if (!w) return 0;
    Display* dpy = (Display*)w->display;
    int count = 0;

    /* Process all pending X11 events */
    XEvent xe;
    while (XPending(dpy)) {
        XNextEvent(dpy, &xe);

        MetaEvent me;
        memset(&me, 0, sizeof(me));
        me.timestamp_ms = 0; /* X11 timestamps are server-time; skip for now */

        switch (xe.type) {
            case ClientMessage: {
                Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
                if ((Atom)xe.xclient.data.l[0] == wm_delete) {
                    me.type = META_EVENT_CLOSE;
                    w->should_close = 1;
                }
                break;
            }
            case DestroyNotify:
                me.type = META_EVENT_CLOSE;
                w->should_close = 1;
                break;
            case ConfigureNotify: {
                int new_w = xe.xconfigure.width;
                int new_h = xe.xconfigure.height;
                if (new_w != w->width || new_h != w->height) {
                    w->width  = new_w;
                    w->height = new_h;
                    me.type = META_EVENT_RESIZE;
                    me.data.resize.width  = new_w;
                    me.data.resize.height = new_h;
                }
                break;
            }
            case KeyPress: {
                KeySym ks = XLookupKeysym(&xe.xkey, 0);
                me.type = META_EVENT_KEY_DOWN;
                me.data.key.keycode = x11_keycode_to_meta(ks);
                me.data.key.mods    = xe.xkey.state;
                break;
            }
            case KeyRelease: {
                KeySym ks = XLookupKeysym(&xe.xkey, 0);
                me.type = META_EVENT_KEY_UP;
                me.data.key.keycode = x11_keycode_to_meta(ks);
                me.data.key.mods    = xe.xkey.state;
                break;
            }
            case ButtonPress:
            case ButtonRelease: {
                if (xe.xbutton.button >= 4 && xe.xbutton.button <= 7) {
                    /* Scroll wheel */
                    me.type = META_EVENT_MOUSE_WHEEL;
                    me.data.mouse_wheel.delta =
                        (xe.xbutton.button == 4 || xe.xbutton.button == 6) ? 1.0 : -1.0;
                } else {
                    me.type = (xe.type == ButtonPress)
                              ? META_EVENT_MOUSE_BTN
                              : META_EVENT_MOUSE_BTN;
                    me.data.mouse_btn.button = (int)xe.xbutton.button;
                    me.data.mouse_btn.action = (xe.type == ButtonPress) ? 1 : 0;
                    me.data.mouse_btn.mods   = xe.xbutton.state;
                }
                break;
            }
            case MotionNotify:
                me.type = META_EVENT_MOUSE_MOVE;
                me.data.mouse_move.x = xe.xmotion.x;
                me.data.mouse_move.y = xe.xmotion.y;
                break;
            default:
                continue; /* skip unhandled */
        }

        if (me.type != META_EVENT_NONE) {
            event_queue_push(&w->event_queue, &me);
            count++;
        }
    }

    return count;
}

int meta_window_next_event(MetaWindow* w, MetaEvent* out) {
    if (!w || !out) return 0;
    return event_queue_pop(&w->event_queue, out);
}

/* ─── Swap buffers ─── */

void meta_window_swap_buffers(MetaWindow* w) {
    if (!w) return;
    Display* dpy = (Display*)w->display;
    XFlush(dpy);
}

/* ─── Queries ─── */

int meta_window_should_close(MetaWindow* w) {
    return w ? w->should_close : 1;
}

void meta_window_set_title(MetaWindow* w, const char* title) {
    if (!w || !title) return;
    size_t len = strlen(title);
    if (len >= sizeof(w->title)) len = sizeof(w->title) - 1;
    memcpy(w->title, title, len);
    w->title[len] = '\0';
    Display* dpy = (Display*)w->display;
    if (dpy) XStoreName(dpy, (Window)w->window, w->title);
}

void meta_window_get_size(MetaWindow* w, int* out_w, int* out_h) {
    if (!w) return;
    if (out_w) *out_w = w->width;
    if (out_h) *out_h = w->height;
}

void meta_window_set_fullscreen(MetaWindow* w, int fullscreen) {
    if (!w) return;
    Display* dpy = (Display*)w->display;
    if (!dpy) return;
    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom full     = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    XEvent xe;
    memset(&xe, 0, sizeof(xe));
    xe.type = ClientMessage;
    xe.xclient.window = (Window)w->window;
    xe.xclient.message_type = wm_state;
    xe.xclient.format = 32;
    xe.xclient.data.l[0] = fullscreen ? 1 : 0; /* _NET_WM_STATE_ADD / _REMOVE */
    xe.xclient.data.l[1] = (long)full;
    XSendEvent(dpy, DefaultRootWindow(dpy), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xe);
}

void* meta_window_native_handle(MetaWindow* w) {
    if (!w) return NULL;
    return (void*)(uintptr_t)w->window;
}

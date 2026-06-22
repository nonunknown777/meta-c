/**
 * window_win32.c — Win32 backend for Brick Window library
 *
 * Uses raw Win32 API (user32 + gdi32). The event queue integrates
 * with the Windows message pump via PeekMessage().
 */

#include "window_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "../../io.h"

/* ─── Forward declare the window procedure ─── */
static LRESULT CALLBACK meta_win32_wndproc(HWND hwnd, UINT msg,
                                           WPARAM wp, LPARAM lp);

/* ─── Store/retrieve MetaWindow* from GWLP_USERDATA ─── */

static void set_window_ptr(HWND hwnd, MetaWindow* w) {
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)w);
}

static MetaWindow* get_window_ptr(HWND hwnd) {
    return (MetaWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
}

/* ─── Key mapping ─── */

static int win32_vk_to_meta(WPARAM vk) {
    switch (vk) {
        case VK_ESCAPE:   return 256;
        case VK_RETURN:   return 257;
        case VK_TAB:      return 258;
        case VK_LEFT:     return 263;
        case VK_RIGHT:    return 262;
        case VK_UP:       return 265;
        case VK_DOWN:     return 264;
        case VK_SHIFT:    return 340;
        case VK_CONTROL:  return 341;
        case VK_MENU:     return 342;
        case VK_SPACE:    return 32;
        default:
            if (vk >= 'A' && vk <= 'Z') return (int)(vk - 'A' + 97);
            if (vk >= '0' && vk <= '9') return (int)vk;
            return 0;
    }
}

/* ─── Window class registration (one-time) ─── */

static const char* META_WIN_CLASS = "BrickWindow";

static int register_window_class(HINSTANCE inst) {
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = meta_win32_wndproc;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"BrickWindow";
    return RegisterClassW(&wc) != 0;
}

/* ─── Window procedure ─── */

static LRESULT CALLBACK meta_win32_wndproc(HWND hwnd, UINT msg,
                                           WPARAM wp, LPARAM lp) {
    MetaWindow* w = get_window_ptr(hwnd);

    switch (msg) {
        case WM_CLOSE:
            if (w) w->should_close = 1;
            return 0;

        case WM_DESTROY:
            if (w) w->should_close = 1;
            PostQuitMessage(0);
            return 0;

        case WM_SIZE: {
            if (w) {
                int new_w = (int)(LOWORD(lp));
                int new_h = (int)(HIWORD(lp));
                if (new_w != w->width || new_h != w->height) {
                    w->width  = new_w;
                    w->height = new_h;
                    MetaEvent me;
                    memset(&me, 0, sizeof(me));
                    me.type = META_EVENT_RESIZE;
                    me.data.resize.width  = new_w;
                    me.data.resize.height = new_h;
                    event_queue_push(&w->event_queue, &me);
                }
            }
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (w) {
                MetaEvent me;
                memset(&me, 0, sizeof(me));
                me.type = META_EVENT_KEY_DOWN;
                me.data.key.keycode = win32_vk_to_meta(wp);
                me.data.key.mods    = (int)wp; /* raw VK for now */
                event_queue_push(&w->event_queue, &me);
            }
            return 0;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP: {
            if (w) {
                MetaEvent me;
                memset(&me, 0, sizeof(me));
                me.type = META_EVENT_KEY_UP;
                me.data.key.keycode = win32_vk_to_meta(wp);
                event_queue_push(&w->event_queue, &me);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            if (w) {
                MetaEvent me;
                memset(&me, 0, sizeof(me));
                me.type = META_EVENT_MOUSE_MOVE;
                me.data.mouse_move.x = (double)(int16_t)(LOWORD(lp));
                me.data.mouse_move.y = (double)(int16_t)(HIWORD(lp));
                event_queue_push(&w->event_queue, &me);
            }
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
            if (w) {
                MetaEvent me;
                memset(&me, 0, sizeof(me));
                me.type = META_EVENT_MOUSE_BTN;
                me.data.mouse_btn.button = (msg == WM_LBUTTONDOWN) ? 1
                                          : (msg == WM_RBUTTONDOWN) ? 2 : 3;
                me.data.mouse_btn.action = 1;
                event_queue_push(&w->event_queue, &me);
            }
            return 0;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            if (w) {
                MetaEvent me;
                memset(&me, 0, sizeof(me));
                me.type = META_EVENT_MOUSE_BTN;
                me.data.mouse_btn.button = (msg == WM_LBUTTONUP) ? 1
                                          : (msg == WM_RBUTTONUP) ? 2 : 3;
                me.data.mouse_btn.action = 0;
                event_queue_push(&w->event_queue, &me);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            if (w) {
                MetaEvent me;
                memset(&me, 0, sizeof(me));
                me.type = META_EVENT_MOUSE_WHEEL;
                short delta = GET_WHEEL_DELTA_WPARAM(wp);
                me.data.mouse_wheel.delta = (double)delta / 120.0;
                event_queue_push(&w->event_queue, &me);
            }
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ─── Creation ─── */

MetaWindow* meta_window_create(
    BlockCtx* block, const char* title, int width, int height, uint32_t flags
) {
    if (!block) return NULL;

    HINSTANCE inst = GetModuleHandleW(NULL);

    static int class_registered = 0;
    if (!class_registered) {
        class_registered = register_window_class(inst);
        if (!class_registered) {
            io_print_string("FATAL: Cannot register Win32 window class", 41);
            return NULL;
        }
    }

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

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD ex_style = WS_EX_APPWINDOW;

    if (flags & META_WINDOW_BORDERLESS) {
        style = WS_POPUP;
    }
    if (flags & META_WINDOW_FULLSCREEN) {
        style = WS_POPUP | WS_VISIBLE;
        ex_style = WS_EX_TOPMOST;
    }

    /* Convert title to wide char */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, w->title, -1, NULL, 0);
    wchar_t* wtitle = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, w->title, -1, wtitle, wlen);

    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, style, FALSE);

    HWND hwnd = CreateWindowExW(
        ex_style, L"BrickWindow", wtitle,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, inst, NULL
    );

    free(wtitle);

    if (!hwnd) {
        io_print_string("FATAL: Cannot create Win32 window", 36);
        return NULL;
    }

    set_window_ptr(hwnd, w);
    w->hinstance = (void*)inst;
    w->hwnd      = (void*)hwnd;
    w->hdc       = (void*)GetDC(hwnd);

    if (!(flags & META_WINDOW_HIDDEN)) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }

    return w;
}

/* ─── Destruction ─── */

void meta_window_destroy(MetaWindow* w) {
    if (!w) return;
    if (w->hwnd) {
        ReleaseDC((HWND)w->hwnd, (HDC)w->hdc);
        DestroyWindow((HWND)w->hwnd);
        w->hwnd = NULL;
    }
}

/* ─── Event polling ─── */

int meta_window_poll_events(MetaWindow* w) {
    if (!w) return 0;
    MSG msg;
    int count = 0;
    while (PeekMessageW(&msg, (HWND)w->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return count;
}

int meta_window_next_event(MetaWindow* w, MetaEvent* out) {
    if (!w || !out) return 0;
    return event_queue_pop(&w->event_queue, out);
}

/* ─── Swap buffers ─── */

void meta_window_swap_buffers(MetaWindow* w) {
    if (!w || !w->hwnd || !w->hdc) return;
    SwapBuffers((HDC)w->hdc);
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
    int wlen = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wtitle = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, wlen);
    SetWindowTextW((HWND)w->hwnd, wtitle);
    free(wtitle);
}

void meta_window_get_size(MetaWindow* w, int* out_w, int* out_h) {
    if (!w) return;
    if (out_w) *out_w = w->width;
    if (out_h) *out_h = w->height;
}

void meta_window_set_fullscreen(MetaWindow* w, int fullscreen) {
    if (!w || !w->hwnd) return;
    HWND hwnd = (HWND)w->hwnd;
    if (fullscreen) {
        SetWindowLongPtrW(hwnd, GWL_STYLE,
                          WS_POPUP | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0,
                     GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN),
                     SWP_FRAMECHANGED);
    } else {
        SetWindowLongPtrW(hwnd, GWL_STYLE,
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0,
                     w->width, w->height,
                     SWP_FRAMECHANGED);
    }
}

void* meta_window_native_handle(MetaWindow* w) {
    if (!w) return NULL;
    return w->hwnd;
}

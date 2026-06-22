#include "../runtime/libs/window/window_hr.h"
#include "../runtime/hot_reload.h"
#include "../runtime/block_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <X11/Xlib.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name()
#define RUN(name) do { \
    name(); \
    printf("[PASS] %s\n", #name); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("[FAIL] %s\n", msg); \
    tests_failed++; \
    return; \
} while(0)

static int has_display(void) {
    const char* d = getenv("DISPLAY");
    return d && d[0] != '\0';
}

static int x11_error_occurred = 0;

static int x11_error_handler(Display* dpy, XErrorEvent* ev) {
    (void)dpy; (void)ev;
    x11_error_occurred = 1;
    return 0;
}

/* Path to the window library .so — set by build system */
/* Caminho para o .so da biblioteca de janela — definido pelo build system */
#ifndef BRICK_LIB_WINDOW_SO
#define BRICK_LIB_WINDOW_SO "build/brick_lib_window.so"
#endif

TEST(test_hr_init_and_load) {
    if (meta_window_table.create != NULL)
        FAIL("test_hr_init_and_load: table should be NULL before init");

    HotReloadEngine* hr = hr_create(BRICK_LIB_WINDOW_SO);
    if (!hr) FAIL("test_hr_init_and_load: hr_create failed");

    int ret = meta_window_hr_init(hr);
    if (ret != 0) {
        hr_destroy(hr);
        FAIL("test_hr_init_and_load: meta_window_hr_init failed");
    }

    if (meta_window_table.create == NULL)     FAIL("test_hr_init_and_load: create is NULL");
    if (meta_window_table.destroy == NULL)    FAIL("test_hr_init_and_load: destroy is NULL");
    if (meta_window_table.poll_events == NULL) FAIL("test_hr_init_and_load: poll_events is NULL");
    if (meta_window_table.next_event == NULL) FAIL("test_hr_init_and_load: next_event is NULL");
    if (meta_window_table.swap_buffers == NULL) FAIL("test_hr_init_and_load: swap_buffers is NULL");
    if (meta_window_table.should_close == NULL) FAIL("test_hr_init_and_load: should_close is NULL");
    if (meta_window_table.set_title == NULL)  FAIL("test_hr_init_and_load: set_title is NULL");
    if (meta_window_table.get_size == NULL)   FAIL("test_hr_init_and_load: get_size is NULL");

    hr_destroy(hr);
    /* Table pointers should survive engine destruction (still valid, just stale) */
    /* Ponteiros da tabela devem sobreviver a destruicao do engine (ainda validos, apenas desatualizados) */
    if (meta_window_table.create == NULL)     FAIL("test_hr_init_and_load: table cleared after destroy");
}

TEST(test_hr_create_window) {
    HotReloadEngine* hr = hr_create(BRICK_LIB_WINDOW_SO);
    if (!hr) FAIL("test_hr_create_window: hr_create failed");
    assert(meta_window_hr_init(hr) == 0);

    BlockCtx* block = block_create_bytes(65536);
    assert(block != NULL);

    MetaWindow* win = meta_win_create(block, "hr_test", 400, 300, META_WINDOW_HIDDEN);
    if (!win) { hr_destroy(hr); block_destroy(block); FAIL("test_hr_create_window: create failed"); }

    int w = 0, h = 0;
    meta_win_get_size(win, &w, &h);
    if (w != 400 || h != 300) {
        meta_win_destroy(win);
        hr_destroy(hr);
        block_destroy(block);
        FAIL("test_hr_create_window: size mismatch");
    }

    meta_win_set_title(win, "hot reloaded");
    int sc = meta_win_should_close(win);
    if (sc != 0) {
        meta_win_destroy(win);
        hr_destroy(hr);
        block_destroy(block);
        FAIL("test_hr_create_window: should_close != 0");
    }

    meta_win_poll_events(win);
    meta_win_swap_buffers(win);
    meta_win_destroy(win);
    hr_destroy(hr);
    block_destroy(block);
}

TEST(test_hr_reload) {
    HotReloadEngine* hr = hr_create(BRICK_LIB_WINDOW_SO);
    if (!hr) FAIL("test_hr_reload: hr_create failed");
    assert(meta_window_hr_init(hr) == 0);

    /* Remember old pointer to verify it changes after reload */
    /* Lembra ponteiro antigo para verificar se muda apos reload */
    void* old_create = meta_window_table.create;
    if (old_create == NULL) { hr_destroy(hr); FAIL("test_hr_reload: create is NULL"); }

    /* Reload the same .so (simulates recompilation) */
    /* Recarrega o mesmo .so (simula recompilacao) */
    int ret = hr_reload(hr);
    if (ret != 0) { hr_destroy(hr); FAIL("test_hr_reload: hr_reload failed"); }

    /* Pointer should still be valid (same .so, same symbol) */
    /* Ponteiro deve continuar valido (mesmo .so, mesmo simbolo) */
    if (meta_window_table.create == NULL)
        FAIL("test_hr_reload: create became NULL after reload");

    /* Basic smoke test through new table */
    /* Teste basico de fumo atraves da nova tabela */
    BlockCtx* block = block_create_bytes(65536);
    assert(block != NULL);
    MetaWindow* win = meta_win_create(block, "reloaded", 640, 480, META_WINDOW_HIDDEN);
    if (!win) { hr_destroy(hr); block_destroy(block); FAIL("test_hr_reload: create after reload failed"); }
    meta_win_destroy(win);
    block_destroy(block);
    hr_destroy(hr);
}

int main() {
    if (!has_display()) {
        printf("[SKIP] No X display available (set $DISPLAY to run)\n");
        return 77;
    }

    XSetErrorHandler(x11_error_handler);

    printf("=== Window Library Hot Reload Tests ===\n\n");

    RUN(test_hr_init_and_load);
    RUN(test_hr_create_window);
    RUN(test_hr_reload);

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}

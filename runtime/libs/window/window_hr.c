/**
 * window_hr.c — Hot reload support for Brick Window library
 *
 * Defines the global function pointer table and hooks into the
 * HotReloadEngine so that all window API calls can be swapped
 * atomically at runtime when the .so is recompiled.
 */

#include "window_hr.h"
#include "../../hot_reload.h"

/* ─── Global function table ─── */

MetaWindowFuncTable meta_window_table = { NULL };

/* ─── Registration ─── */

int meta_window_hr_init(HotReloadEngine* hr) {
    if (!hr) return -1;

    hr_register_func(hr, "meta_window_create",       &meta_window_table.create);
    hr_register_func(hr, "meta_window_destroy",       &meta_window_table.destroy);
    hr_register_func(hr, "meta_window_poll_events",   &meta_window_table.poll_events);
    hr_register_func(hr, "meta_window_next_event",    &meta_window_table.next_event);
    hr_register_func(hr, "meta_window_swap_buffers",  &meta_window_table.swap_buffers);
    hr_register_func(hr, "meta_window_should_close",  &meta_window_table.should_close);
    hr_register_func(hr, "meta_window_set_title",     &meta_window_table.set_title);
    hr_register_func(hr, "meta_window_get_size",      &meta_window_table.get_size);
    hr_register_func(hr, "meta_window_set_fullscreen",&meta_window_table.set_fullscreen);
    hr_register_func(hr, "meta_window_native_handle", &meta_window_table.native_handle);

    return hr_load_initial(hr);
}

/* ─── Convenience: create, init, start watching ─── */

HotReloadEngine* meta_window_hr_start(const char* so_path) {
    HotReloadEngine* hr = hr_create(so_path);
    if (!hr) return NULL;

    if (meta_window_hr_init(hr) != 0) {
        hr_destroy(hr);
        return NULL;
    }

    if (hr_start_watching(hr) != 0) {
        hr_destroy(hr);
        return NULL;
    }

    return hr;
}

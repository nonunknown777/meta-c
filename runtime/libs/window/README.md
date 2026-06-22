# Brick Window Library

Cross-platform window creation and input for Brick.

## Backends

| Platform | Backend     | Status    |
|----------|-------------|-----------|
| Linux    | X11 (Xlib)  | Beta      |
| Linux    | Wayland     | Planned   |
| Windows  | Win32       | Beta      |
| macOS    | N/A         | TBD       |

## Dependencies

- **Linux:** `libX11-dev` (Ubuntu: `apt install libX11-dev`)
- **Windows:** Win32 SDK (included with MSVC/MinGW)

## Quickstart

```brick
using Window

block global = 64MB

fn main() {
    Window w = Window.create("Hello", 800, 600, Window.RESIZABLE)
    if w == null { error("no window") }

    while !w.should_close() {
        w.poll_events()
        // render here
        w.swap_buffers()
    }

    w.destroy()
}
```

## Build (manual)

```bash
# Linux
gcc -O3 basic.c runtime/libs/window/window_linux.c runtime/block_memory.c -lX11 -lm -o basic

# Windows (MinGW)
gcc -O3 basic.c runtime/libs/window/window_win32.c runtime/block_memory.c -lgdi32 -o basic.exe
```

## API Reference

See `window.h` for the full C API. Brick binding maps as follows:

| Brick                                 | C function                          |
|----------------------------------------|-------------------------------------|
| `Window.create(title, w, h, flags)`    | `meta_window_create(...)`           |
| `w.destroy()`                          | `meta_window_destroy(w)`            |
| `w.poll_events()`                      | `meta_window_poll_events(w)`        |
| `w.next_event(e)`                      | `meta_window_next_event(w, &e)`     |
| `w.swap_buffers()`                     | `meta_window_swap_buffers(w)`       |
| `w.should_close()`                     | `meta_window_should_close(w)`       |
| `w.set_title(title)`                   | `meta_window_set_title(w, title)`   |
| `w.get_size()`                         | `meta_window_get_size(w, &w, &h)`   |
| `w.set_fullscreen(bool)`               | `meta_window_set_fullscreen(w, b)`  |
| `w.native_handle()`                    | `meta_window_native_handle(w)`      |

## Performance Notes

- Event queue uses a fixed-size ring buffer (no allocations at runtime)
- X11 backend uses `XFlush` (not `XSync`) — no round-trip per frame
- Win32 backend uses `PeekMessage` (not `GetMessage`) — non-blocking poll
- No virtual dispatch overhead — direct function calls via codegen

# Next Step — Window Library: Test & Verify
# Próximo Passo — Biblioteca Window: Testar & Verificar

## Immediate
## Imediato

1. Build and run `test_libs_window` on Linux/X11 — verify all 15 tests pass
2. Verify SCons test alias works (`scons test` includes window tests)
3. Test edge cases manually (e.g., running without $DISPLAY → skip code 77)
4. Ensure `scons profile=sanitize` works with window library

1. Build e execute `test_libs_window` no Linux/X11 — verifique todos os 15 testes passam
2. Verifique o alias SCons test funciona (`scons test` inclui testes window)
3. Teste casos extremos manualmente (ex.: executar sem $DISPLAY → skip code 77)
4. Garanta que `scons profile=sanitize` funciona com a biblioteca window

## Future
## Futuro

- Refactor `meta_window_create` to use Brick block allocator instead of calloc
- Add Wayland backend
- macOS/Cocoa backend
- Input library (keyboard, mouse, gamepad)
- Audio, File I/O, Networking, Math libraries

- Refatorar `meta_window_create` para usar alocador de blocos Brick em vez de calloc
- Adicionar backend Wayland
- Backend macOS/Cocoa
- Biblioteca Input (teclado, mouse, gamepad)
- Bibliotecas Audio, File I/O, Networking, Math

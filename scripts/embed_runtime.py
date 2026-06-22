#!/usr/bin/env python3
"""embed_runtime.py — Gera embedded_runtime.h/.cpp com fontes C embutidos.

Uso:
  embed_runtime.py --target linux|windows --runtime-dir <dir> --out-h <file> --out-cpp <file> [--has-x11]
"""
import argparse, os, sys

def raw_delim(s):
    """Gera delimitador pra C++ raw string literal R\"...\"(content)...\".
    Garante que )delim\" não aparece no conteúdo."""
    delim = ''
    while f'){delim}"' in s:
        delim += 'X'
    return delim

def make_c_identifier(path):
    """block_memory.h → _runtime_block_memory_h"""
    name = path.replace('/', '_').replace('.', '_')
    return '_runtime_' + name

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--target', required=True, choices=['linux', 'windows'])
    ap.add_argument('--runtime-dir', required=True)
    ap.add_argument('--out-h', required=True)
    ap.add_argument('--out-cpp', required=True)
    ap.add_argument('--has-x11', action='store_true')
    args = ap.parse_args()

    runtime_dir = args.runtime_dir

    # ─── Platform file lists ────────────────────────────────────
    # ─── Listas de arquivos por plataforma ──────────────────────
    always = [
        'block_memory.h', 'block_memory.c',
        'io.h', 'io.c',
    ]

    if args.target == 'linux':
        extra_c = ['hot_reload.c', 'libs/window/window_linux.c']
        extra_h = ['hot_reload.h', 'libs/window/window.h',
                   'libs/window/window_internal.h', 'libs/window/window_hr.h']
        extra_c_hr = ['libs/window/window_hr.c']  # only with hot reload
                                                    # apenas com hot reload
        flags_required = ['-ldl', '-lpthread']
        flags_optional = ['-lX11'] if args.has_x11 else []
        cc = 'gcc'
        target_name = 'linux'
    else:
        extra_c = ['libs/window/window_win32.c']
        extra_h = ['libs/window/window.h', 'libs/window/window_internal.h']
        extra_c_hr = []
        flags_required = ['-luser32', '-lgdi32']
        flags_optional = []
        cc = 'x86_64-w64-mingw32-gcc'
        target_name = 'windows'

    files = []
    for f in always + extra_h + extra_c + extra_c_hr:
        files.append(f)

    # ─── Read all files ─────────────────────────────────────────
    # ─── Le todos os arquivos ───────────────────────────────────
    entries = []
    for f in files:
        path = os.path.join(runtime_dir, f)
        if not os.path.exists(path):
            print(f"[embed] WARNING: {path} not found, skipping", file=sys.stderr)
            print(f"[embed] AVISO: {path} nao encontrado, pulando", file=sys.stderr)
            continue
        with open(path, 'rb') as fp:
            data = fp.read()
        # Normalize line endings
        # Normaliza quebras de linha
        text = data.decode('utf-8').replace('\r\n', '\n')
        entries.append((f, text))

    # ─── Generate header ────────────────────────────────────────
    # ─── Gera header ─────────────────────────────────────────────
    with open(args.out_h, 'w', encoding='utf-8') as h:
        h.write('#pragma once\n')
        h.write('#include <cstddef>\n')
        h.write('#include <cstdint>\n')
        h.write('\n')
        h.write('namespace brick { namespace embedded {\n')
        h.write('\n')
        h.write('struct EmbeddedFile {\n')
        h.write('    const char* name;\n')
        h.write('    const char* content;\n')
        h.write('    size_t length;\n')
        h.write('};\n')
        h.write('\n')
        h.write('struct RuntimeInfo {\n')
        h.write('    const char* target;\n')
        h.write('    const char* cc;\n')
        h.write('    int num_sources;\n')
        h.write('    const EmbeddedFile* sources;\n')
        h.write('    int num_flags;\n')
        h.write('    const char* const* flags;\n')
        h.write('};\n')
        h.write('\n')
        h.write('extern const EmbeddedFile _runtime_sources[];\n')
        h.write('extern const char* _runtime_flags[];\n')
        h.write('extern const RuntimeInfo runtime_info;\n')
        h.write('\n')
        for name, text in entries:
            ident = make_c_identifier(name)
            h.write(f'extern const char {ident}[];\n')
            h.write(f'extern const size_t {ident}_len;\n')
        h.write('\n')
        h.write('} }\n')

    # ─── Generate cpp ───────────────────────────────────────────
    # ─── Gera cpp ────────────────────────────────────────────────
    with open(args.out_cpp, 'w', encoding='utf-8') as cpp:
        cpp.write('#include "embedded_runtime.h"\n')
        cpp.write('\n')
        cpp.write('namespace brick { namespace embedded {\n')
        cpp.write('\n')

        for name, text in entries:
            ident = make_c_identifier(name)
            delim = raw_delim(text)
            cpp.write(f'const char {ident}[] = R"{delim}(\n')
            cpp.write(text)
            if text and not text.endswith('\n'):
                cpp.write('\n')
            cpp.write(f'){delim}";\n')
            cpp.write(f'const size_t {ident}_len = sizeof({ident}) - 1;\n')
            cpp.write('\n')

        cpp.write('const EmbeddedFile _runtime_sources[] = {\n')
        for name, text in entries:
            ident = make_c_identifier(name)
            cpp.write(f'    {{"{name}", {ident}, {ident}_len}},\n')
        cpp.write('};\n')
        cpp.write('\n')

        all_flags = flags_required + flags_optional
        cpp.write('const char* _runtime_flags[] = {\n')
        for flag in all_flags:
            cpp.write(f'    "{flag}",\n')
        cpp.write('};\n')
        cpp.write('\n')

        cpp.write('const RuntimeInfo runtime_info = {\n')
        cpp.write(f'    "{target_name}",\n')
        cpp.write(f'    "{cc}",\n')
        cpp.write(f'    {len(entries)},\n')
        cpp.write(f'    _runtime_sources,\n')
        cpp.write(f'    {len(all_flags)},\n')
        cpp.write(f'    _runtime_flags,\n')
        cpp.write('};\n')
        cpp.write('\n')
        cpp.write('} }\n')

    print(f"[embed] {args.target}: {len(entries)} files, {len(all_flags)} flags -> "
          f"{os.path.basename(args.out_h)}, {os.path.basename(args.out_cpp)}")

if __name__ == '__main__':
    main()

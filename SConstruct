# =============================================================================
# Brick Build System (SCons) — Entry Point
# Sistema de Build Brick (SCons) — Ponto de Entrada
# =============================================================================
import os, subprocess, sys
from os.path import join

Help("""
Brick Build System
Sistema de Build Brick
===================
Targets:
Alvos:
  scons                    Build release (-O3) for native target
  scons                    Build release (-O3) para alvo nativo
  scons profile=debug      Debug build (-g -O0 -DDEBUG)
  scons profile=debug      Build debug (-g -O0 -DDEBUG)
  scons profile=sanitize   ASAN+UBSan build
  scons profile=sanitize   Build ASAN+UBSan
  scons test               Build and run all tests (native only)
  scons test               Build e executa todos os testes (nativo apenas)
  scons install            Install to prefix (default: /usr/local)
  scons install            Instala no prefixo (padrao: /usr/local)

Profiles:
Perfis:
  release     -std=c++20 -O3 -Wall
  debug       -std=c++20 -g -O0 -DDEBUG
  sanitize    -std=c++20 -g -O1 -fsanitize=address,undefined (linux only)
  pgo-gen     -std=c++20 -O3 -Wall -fprofile-generate (PGO generate, linux only)
  pgo-use     -std=c++20 -O3 -Wall -fprofile-use (PGO use, linux only)

Options:
Opcoes:
  prefix=/usr/local        Install prefix
  prefix=/usr/local        Prefixo de instalacao
  compiler=gcc             Force gcc (default: auto-detect)
  compiler=gcc             Forca gcc (default: auto-detect)
  compiler=clang           Force clang
  compiler=clang           Forca clang
  visualizer=no            Skip ncurses / PDCurses visualizer
  visualizer=no            Pula visualizador ncurses / PDCurses

Target:
Alvo:
  scons target=windows     Native Windows build (default on Windows)
  scons target=windows     Build nativo Windows (padrao no Windows)
  scons target=linux       Explicit Linux build (default on Linux)
  scons target=linux       Build Linux explicito (padrao no Linux)

""")

# ═════════════════════════════════════════════════════════════════════════════
# 1. ARGUMENTS
# 1. ARGUMENTOS
# ═════════════════════════════════════════════════════════════════════════════
is_windows_host = sys.platform == 'win32'
default_target = 'windows' if is_windows_host else 'linux'

target   = ARGUMENTS.get('target', default_target)
profile  = ARGUMENTS.get('profile', 'release')
compiler = ARGUMENTS.get('compiler', 'auto')
viz_opt  = ARGUMENTS.get('visualizer', 'yes')
prefix   = ARGUMENTS.get('prefix', '/usr/local' if not is_windows_host else 'C:/Brick')

is_cross    = target not in ('linux', 'windows')
prog_suffix = ''
platform_libs = []

def find_prog(names):
    for n in names:
        try:
            subprocess.run([n, '--version'], capture_output=True, check=True)
            return n
        except OSError:
            continue
    return names[0]

# ═════════════════════════════════════════════════════════════════════════════
# 2. COMPILER DETECTION (per target)
# 2. DETECCAO DE COMPILADOR (por alvo)
# ═════════════════════════════════════════════════════════════════════════════
if target == 'windows':
    prog_suffix = '.exe'
    if is_windows_host:
        if compiler == 'clang':
            cxx, cc = 'clang++', 'clang'
        elif compiler == 'gcc':
            cxx, cc = 'g++', 'gcc'
        else:
            cxx = find_prog(['g++', 'clang++', 'g++'])
            cc  = find_prog(['gcc', 'clang', 'gcc'])
        print(f"[build] target=windows (native)  CXX={cxx}  CC={cc}")
    else:
        cxx = 'x86_64-w64-mingw32-g++'
        cc  = 'x86_64-w64-mingw32-gcc'
        print(f"[build] target=windows (cross)  CXX={cxx}")
        is_cross = True

elif target == 'linux':
    if compiler == 'clang':
        cxx, cc = 'clang++', 'clang'
    elif compiler == 'gcc':
        cxx, cc = 'g++', 'gcc'
    else:
        cxx = find_prog(['g++', 'clang++'])
        cc  = find_prog(['gcc', 'clang'])
    platform_libs = ['dl', 'pthread']
    print(f"[build] target=linux  CXX={cxx}  CC={cc}")

else:
    triple = target
    cxx = f'{triple}-g++'
    cc  = f'{triple}-gcc'
    platform_libs = ['dl', 'pthread']
    print(f"[build] target={triple}  CXX={cxx}  CC={cc}")
    is_cross = True

# ═════════════════════════════════════════════════════════════════════════════
# 3. BASE ENVIRONMENT
# 3. AMBIENTE BASE
# ═════════════════════════════════════════════════════════════════════════════
if target == 'windows':
    # Force POSIX build semantics so SCons uses GCC-style flags (-o, -c, -I)
    # instead of MSVC-style (/Fo, /c, /I) when compiling with MinGW g++
    env = Environment(
        CXX=cxx, CC=cc,
        PLATFORM='posix',
        CXXFLAGS=['-std=c++20', '-Wall'],
        CFLAGS=['-std=c11', '-Wall'],
        LIBS=platform_libs,
        CPPPATH=['#src', '#runtime'],
        LIBPATH=['#build'],
        ENV=os.environ,
    )
else:
    env = Environment(
        CXX=cxx, CC=cc,
        CXXFLAGS=['-std=c++20', '-Wall'],
        CFLAGS=['-std=c11', '-Wall'],
        LIBS=platform_libs,
        CPPPATH=['#src', '#runtime'],
        LIBPATH=['#build'],
        ENV=os.environ,
    )
if prog_suffix:
    env['PROGSUFFIX'] = prog_suffix

# ═════════════════════════════════════════════════════════════════════════════
# 4. BUILD PROFILE
# 4. PERFIL DE BUILD
# ═════════════════════════════════════════════════════════════════════════════
if profile == 'release':
    env.Append(CXXFLAGS=['-O3'], CFLAGS=['-O3'])
elif profile == 'debug':
    env.Append(CXXFLAGS=['-g', '-O0', '-DDEBUG'],
               CFLAGS=['-g', '-O0', '-DDEBUG'])
elif profile == 'sanitize':
    env.Append(CXXFLAGS=['-g', '-O1', '-fsanitize=address', '-fsanitize=undefined'],
               CFLAGS=['-g', '-O1', '-fsanitize=address', '-fsanitize=undefined'],
               LINKFLAGS=['-fsanitize=address', '-fsanitize=undefined'])
elif profile == 'pgo-gen':
    print("[build] PGO generate mode")
    print("[build] Modo PGO generate")
    env.Append(CXXFLAGS=['-O3', '-fprofile-generate'],
               CFLAGS=['-O3', '-fprofile-generate'],
               LINKFLAGS=['-fprofile-generate'])
elif profile == 'pgo-use':
    print("[build] PGO use mode")
    print("[build] Modo PGO use")
    env.Append(CXXFLAGS=['-O3', '-fprofile-use', '-fprofile-correction'],
               CFLAGS=['-O3', '-fprofile-use', '-fprofile-correction'],
               LINKFLAGS=['-fprofile-use', '-fprofile-correction'])
else:
    print(f"[build] Unknown profile '{profile}', falling back to release")
    print(f"[build] Perfil desconhecido '{profile}', usando release como fallback")
    env.Append(CXXFLAGS=['-O3'], CFLAGS=['-O3'])

# ═════════════════════════════════════════════════════════════════════════════
# 5. CACHE
# ═════════════════════════════════════════════════════════════════════════════
CacheDir('.scons_cache')

# ═════════════════════════════════════════════════════════════════════════════
# 6. PLATFORM FEATURES
# 6. FUNCIONALIDADES DE PLATAFORMA
# ═════════════════════════════════════════════════════════════════════════════
has_ncurses    = False
has_visualizer = False
has_hotreload  = False
has_x11        = False
has_pdcurses   = False

if target == 'windows':
    # Windows: hot reload via LoadLibrary (always available)
    has_hotreload = True
    # Visualizer: try PDCurses if available
    if viz_opt != 'no':
        try:
            subprocess.run(['pkg-config', 'pdcurses'], capture_output=True, check=True)
            has_pdcurses  = True
            has_visualizer = True
        except (OSError, subprocess.CalledProcessError):
            print("[build] PDCurses not found, skipping visualizer on Windows")
            print("[build] PDCurses nao encontrado, pulando visualizador no Windows")
    env.Append(CPPPATH=['#visualizer'])
else:
    # Linux / other Unix
    has_hotreload = True
    if viz_opt != 'no':
        try:
            subprocess.run(['pkg-config', 'ncurses'], capture_output=True, check=True)
            has_ncurses    = True
            has_visualizer = True
        except (OSError, subprocess.CalledProcessError):
            print("[build] ncurses not found, skipping visualizer")
            print("[build] ncurses nao encontrado, pulando visualizador")
    env.Append(CPPPATH=['#visualizer'])

if target == 'linux':
    try:
        subprocess.run(['pkg-config', '--exists', 'x11'], capture_output=True, check=True)
        has_x11 = True
        print("[build] X11 found, building window library")
        print("[build] X11 encontrado, construindo biblioteca de janela")
    except (OSError, subprocess.CalledProcessError):
        print("[build] X11 not found, skipping window library")
        print("[build] X11 nao encontrado, pulando biblioteca de janela")
elif target == 'windows':
    # Win32 backend is embedded via runtime — no separate library build needed
    # Backend Win32 e embutido via runtime — sem build separado de biblioteca
    pass

if has_visualizer:
    env.Append(CFLAGS=['-DBRICK_TRACK_BLOCKS'],
               CXXFLAGS=['-DBRICK_TRACK_BLOCKS'])

# ═════════════════════════════════════════════════════════════════════════════
# 7. EXPORT (shared context for all SConscripts)
# 7. EXPORTACAO (contexto compartilhado para todos os SConscripts)
# ═════════════════════════════════════════════════════════════════════════════
Export(
    'env', 'platform_libs', 'prog_suffix',
    'has_ncurses', 'has_pdcurses', 'has_visualizer', 'has_hotreload', 'has_x11',
    'is_cross', 'prefix', 'target', 'profile',
)

# ═════════════════════════════════════════════════════════════════════════════
# 8. SUB-BUILDS (libraries first, then programs, then tests)
# 8. SUB-BUILDS (bibliotecas primeiro, depois programas, depois testes)
# ═════════════════════════════════════════════════════════════════════════════
SConscript('runtime/SConscript', chdir=True)

if target == 'linux' and has_x11:
    SConscript('runtime/libs/window/SConscript', chdir=True)

if has_visualizer:
    SConscript('visualizer/SConscript', chdir=True)

SConscript('src/SConscript', chdir=True)
SConscript('tests/SConscript', chdir=True)

# ═════════════════════════════════════════════════════════════════════════════
# 9. ALIASES
# ═════════════════════════════════════════════════════════════════════════════
prog_path = join('build', 'brick' + prog_suffix)
env.Alias('build', prog_path)
env.Alias('install', env.Install(join(prefix, 'bin'), prog_path))
Default('build')

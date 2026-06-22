# =============================================================================
# Brick Build System (SCons) — Entry Point
# Sistema de Build Brick (SCons) — Ponto de Entrada
# =============================================================================
import os, subprocess
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

Cross-compilation:
Cross-compilacao:
  scons target=windows     Cross-compile to Windows (needs mingw-w64)
  scons target=windows     Cross-compila para Windows (requer mingw-w64)
  scons target=linux       Explicit native build (default)
  scons target=linux       Build nativo explicito (padrao)

Profiles:
Perfis:
  release   -std=c++20 -O3 -Wall
  debug     -std=c++20 -g -O0 -DDEBUG
  sanitize  -std=c++20 -g -O1 -fsanitize=address,undefined

Options:
Opcoes:
  prefix=/usr/local        Install prefix
  prefix=/usr/local        Prefixo de instalacao
  compiler=gcc             Force gcc (default: auto-detect)
  compiler=gcc             Forca gcc (padrao: auto-detect)
  compiler=clang           Force clang
  compiler=clang           Forca clang
  visualizer=no            Skip ncurses visualizer
  visualizer=no            Pula visualizador ncurses
""")

# ═════════════════════════════════════════════════════════════════════════════
# 1. ARGUMENTS
# 1. ARGUMENTOS
# ═════════════════════════════════════════════════════════════════════════════
target   = ARGUMENTS.get('target', 'linux')
profile  = ARGUMENTS.get('profile', 'release')
compiler = ARGUMENTS.get('compiler', 'auto')
viz_opt  = ARGUMENTS.get('visualizer', 'yes')
prefix   = ARGUMENTS.get('prefix', '/usr/local')

is_cross    = target not in ('linux', '')
prog_suffix = ''
platform_libs = []

# ═════════════════════════════════════════════════════════════════════════════
# 2. COMPILER DETECTION (per target)
# 2. DETECCAO DE COMPILADOR (por alvo)
# ═════════════════════════════════════════════════════════════════════════════
if target == 'windows':
    cxx = 'x86_64-w64-mingw32-g++'
    cc  = 'x86_64-w64-mingw32-gcc'
    prog_suffix = '.exe'
    print(f"[build] target=windows  CXX={cxx}")

elif target == 'linux':
    def find_prog(names):
        for n in names:
            try:
                subprocess.run([n, '--version'], capture_output=True, check=True)
                return n
            except OSError:
                continue
        return names[0]
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

if target != 'windows':
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

if has_visualizer:
    env.Append(CFLAGS=['-DBRICK_TRACK_BLOCKS'],
               CXXFLAGS=['-DBRICK_TRACK_BLOCKS'])

# ═════════════════════════════════════════════════════════════════════════════
# 7. EXPORT (shared context for all SConscripts)
# 7. EXPORTACAO (contexto compartilhado para todos os SConscripts)
# ═════════════════════════════════════════════════════════════════════════════
Export(
    'env', 'platform_libs', 'prog_suffix',
    'has_ncurses', 'has_visualizer', 'has_hotreload', 'has_x11',
    'is_cross', 'prefix', 'target', 'profile',
)

# ═════════════════════════════════════════════════════════════════════════════
# 8. SUB-BUILDS (libraries first, then programs, then tests)
# 8. SUB-BUILDS (bibliotecas primeiro, depois programas, depois testes)
# ═════════════════════════════════════════════════════════════════════════════
SConscript('runtime/SConscript', chdir=True)

if has_x11:
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

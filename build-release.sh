#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RELEASE_DIR="$SCRIPT_DIR/build/release"
BUILD_DIR="$SCRIPT_DIR/build"

echo "╔═══════════════════════════════════════════════╗"
echo "║     Brick Release Builder v0.1              ║"
echo "╚═══════════════════════════════════════════════╝"

# ─── 1. Build release profile ──────────────────────────────────────────────
echo ""
echo "[1/3] Building brick (release)..."
cd "$SCRIPT_DIR"
scons profile=release -j$(nproc) 2>&1 | sed 's/^/      /'

# ─── 2. Assemble release ───────────────────────────────────────────────────
echo ""
echo "[2/3] Assembling release..."
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

cp "$BUILD_DIR/brick" "$RELEASE_DIR/brick"
strip "$RELEASE_DIR/brick" 2>/dev/null || true
echo "      brick  ($(du -h "$RELEASE_DIR/brick" | cut -f1))"

# ─── 3. Package VS Code extension ──────────────────────────────────────────
echo ""
echo "[3/3] Packaging VS Code extension..."
VSIX="$RELEASE_DIR/brick-language.vsix"

if [ -d "$SCRIPT_DIR/vscode-ext" ]; then
    cd "$SCRIPT_DIR/vscode-ext"

    if [ ! -f out/extension.js ]; then
        echo "      Compiling extension..."
        npm install --silent 2>/dev/null
        npm run compile --silent 2>/dev/null || true
        if [ -d server ]; then
            cd server
            npm install --silent 2>/dev/null
            npm run compile --silent 2>/dev/null || true
            cd ..
        fi
    fi

    # Try vsce, fallback to manual copy
    if command -v npx &>/dev/null && npx --yes vsce --version &>/dev/null; then
        npx --yes vsce package --out "$VSIX" 2>&1 | sed 's/^/      /'
        echo "      $(basename "$VSIX")  ($(du -h "$VSIX" | cut -f1))"
    else
        # Manual: copy extension folder (user runs npm install to activate)
        rsync -a --exclude='node_modules' \
                 --exclude='.vscode' \
                 --exclude='src' \
                 --exclude='tsconfig.json' \
                 --exclude='package-lock.json' \
                 "$SCRIPT_DIR/vscode-ext/" "$RELEASE_DIR/vscode-ext/"
        if [ -d "$SCRIPT_DIR/vscode-ext/server" ]; then
            mkdir -p "$RELEASE_DIR/vscode-ext/server"
            rsync -a --exclude='node_modules' \
                     --exclude='src' \
                     --exclude='tsconfig.json' \
                     --exclude='package-lock.json' \
                     "$SCRIPT_DIR/vscode-ext/server/" "$RELEASE_DIR/vscode-ext/server/"
        fi
        echo "      vscode-ext/  (run 'npm install' inside to activate, or install vsce for .vsix)"
    fi
fi

echo ""
echo ""
echo "═══════════════════════════════════════════════════"
echo "  Release ready: $RELEASE_DIR"
echo "    brick                    (compilador + visualizer)"
echo "    brick-language.vsix      (extensão VS Code)"
echo "═══════════════════════════════════════════════════"
echo ""
echo "  Dependências necessárias:"
echo "    - ncurses   (para brick --visualize / --attach)"
echo "    - gcc, make (para brick build / run)"
echo "    - libX11    (Linux, apenas com window library)"
echo "═══════════════════════════════════════════════════"

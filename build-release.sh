#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RELEASE_DIR="$SCRIPT_DIR/build/release"
BUILD_DIR="$SCRIPT_DIR/build"

echo "╔═══════════════════════════════════════════════╗"
echo "║     Brick Release Builder v0.3              ║"
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

cp "$BUILD_DIR/brick" "$RELEASE_DIR/brick_x11"
strip "$RELEASE_DIR/brick_x11" 2>/dev/null || true
echo "      brick_x11  ($(du -h "$RELEASE_DIR/brick_x11" | cut -f1))"

# ─── 3. Package VS Code extension ──────────────────────────────────────────
echo ""
echo "[3/3] Packaging VS Code extension..."
VSIX="$RELEASE_DIR/brick-language.vsix"

if [ -d "$SCRIPT_DIR/vscode-ext" ]; then
    cd "$SCRIPT_DIR/vscode-ext"

    # Always compile — ensures fresh build even on CI
    echo "      Compiling extension..."
    cd "$SCRIPT_DIR/vscode-ext"
    npm install 2>&1 | sed 's/^/      /'
    npm run compile 2>&1 | sed 's/^/      /'
    if [ -d server ]; then
        cd server
        npm install 2>&1 | sed 's/^/      /'
        npm run compile 2>&1 | sed 's/^/      /'
        cd ..
    fi

    # Package .vsix with vsce (auto-installed via npx)
    echo "      Packaging .vsix..."
    npx --yes vsce package --out "$VSIX" 2>&1 | sed 's/^/      /'
    if [ -f "$VSIX" ]; then
        echo "      $(basename "$VSIX")  ($(du -h "$VSIX" | cut -f1))"
    else
        echo "      ERROR: .vsix was not created!"
        exit 1
    fi
fi

echo ""
echo ""
echo "═══════════════════════════════════════════════════"
echo "  Release ready: $RELEASE_DIR"
echo "    brick_x11                (compilador + visualizer + X11)"
echo "    brick-language.vsix      (extensão VS Code)"
echo "═══════════════════════════════════════════════════"
echo ""
echo "  Dependências necessárias:"
echo "    - ncurses   (para brick --visualize / --attach)"
echo "    - gcc, make (para brick build / run)"
echo "    - libX11    (Linux, apenas com window library)"
echo "═══════════════════════════════════════════════════"

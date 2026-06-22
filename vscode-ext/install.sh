#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EXT_NAME="brick.brick-language"
VSCODE_EXT_DIR="$HOME/.vscode-oss/extensions"
TARGET_DIR="$VSCODE_EXT_DIR/$EXT_NAME"

echo "Instalando dependencias da extensao..."
cd "$SCRIPT_DIR"
npm install
npm run compile

echo "Compilando LSP server..."
cd "$SCRIPT_DIR/server"
npm install
npm run compile

cd "$SCRIPT_DIR"

mkdir -p "$VSCODE_EXT_DIR"
rm -rf "$TARGET_DIR"
ln -sf "$SCRIPT_DIR" "$TARGET_DIR"

echo ""
echo "Brick extension installed with LSP!"
echo "Recarregue o VS Code: Ctrl+Shift+P -> Developer: Reload Window"

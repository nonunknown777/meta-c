#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
BRICK="$BUILD_DIR/brick"
RUNTIME_DIR="$PROJECT_DIR/runtime"

BRC_FILE="$1"
if [ -z "$BRC_FILE" ]; then
    echo "Uso: $0 <arquivo.brc>"
    exit 1
fi

BASENAME="$(basename "$BRC_FILE" .brc)"
C_FILE="$BUILD_DIR/${BASENAME}.c"
BIN_FILE="$BUILD_DIR/${BASENAME}"

echo "--- $BASENAME ---"

"$BRICK" "$BRC_FILE" -o "$C_FILE"

gcc -O3 -I"$RUNTIME_DIR" "$C_FILE" \
    "$RUNTIME_DIR/block_memory.c" \
    "$RUNTIME_DIR/hot_reload.c" \
    -o "$BIN_FILE" -ldl

"$BIN_FILE"

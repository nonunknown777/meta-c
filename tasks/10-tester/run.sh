#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
konsole --new-tab \
        --title "🔬 Brick: Tester" \
        --workdir "$SCRIPT_DIR" \
        -e opencode

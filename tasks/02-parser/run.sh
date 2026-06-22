#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
konsole --new-tab \
        --title "🌳 Brick: Parser" \
        --workdir "$SCRIPT_DIR" \
        -e opencode

#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
konsole --new-tab \
        --title "💻 Brick: VSCoder" \
        --workdir "$SCRIPT_DIR" \
        -e opencode

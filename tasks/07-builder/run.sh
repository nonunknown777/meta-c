#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
konsole --new-tab \
        --title "🏗️ Brick: Builder" \
        --workdir "$(dirname "$SCRIPT_DIR")" \
        -e opencode

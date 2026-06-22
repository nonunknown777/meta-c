#!/bin/bash
# Brick: Abre TODAS as tasks em um UNICO Konsole com varias abas

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OPENCODE="$(which opencode 2>/dev/null || echo "$HOME/.opencode/bin/opencode")"

TABS_FILE=$(mktemp)

echo "Abrindo workspace Brick em um unico Konsole..."

cat > "$TABS_FILE" << EOF
title: Root ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR;;
title: Lexer ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/01-lexer;;
title: Parser ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/02-parser;;
title: Codegen ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/03-codegen;;
title: Runtime ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/04-runtime;;
title: Hot Reload ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/05-hotreload;;
title: Visualizer ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/06-visualizer;;
title: Builder ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/07-builder;;
title: VSCoder ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/08-vscoder;;
title: Debugger ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/09-debugger;;
title: Tester ;; command: $OPENCODE ;; workdir: $SCRIPT_DIR/tasks/10-tester;;
EOF

konsole --tabs-from-file "$TABS_FILE" &
disown

sleep 1
rm -f "$TABS_FILE"

echo "Konsole aberto com 11 abas (Root + 10 tasks)."

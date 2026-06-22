# Brick VS Code Extension

The VS Code extension provides syntax highlighting, snippets, language server protocol (LSP) integration, debugger support, and a memory block visualizer — all for the Brick language.

---

## Quick Start

### Installation

```bash
cd vscode-ext/
npm install        # Install dependencies
npm run compile    # Compile TypeScript
```

Then press `F5` in VS Code to launch a new Extension Development Host window, or copy the extension to `~/.vscode/extensions/`.

### Prerequisites

- **VS Code 1.80+**
- **C++ tools extension**: `ms-vscode.cpptools` (auto-installed as dependency)
- **Brick compiler**: Built with `scons` in the project root
- **GDB**: For debugging support

---

## Features

### 1. Syntax Highlighting

Files with `.brc` extension are automatically recognized as Brick. The TextMate grammar provides semantic highlighting for:

- **Keywords**: `package`, `struct`, `fn`, `if`, `while`, `for`, `return`, `block`, `reset` — distinct color
- **Types**: `int`, `float`, `bool`, `char`, `String`, `void` — type color
- **Struct names**: `Player`, `Enemy`, etc. — class/type color
- **Functions/Methods**: coloring for function calls and declarations
- **Strings**: quoted string coloring with escape sequence support
- **Comments**: `//` line comments
- **Operators**: `+`, `-`, `*`, `/`, `==`, `!=`, `@`, `->` — operator color
- **Numbers**: int and float literals — numeric color

### 2. Snippets

The extension includes code snippets for common patterns:

| Prefix | Expands To |
|--------|------------|
| `pack` | `package NAME` |
| `us` | `using NAME` |
| `struct` | `struct Name { }` |
| `struct-ext` | `struct Name extends Base { }` |
| `iface` | `interface Name { }` |
| `fn` | `fn name(args) { }` |
| `fn-ret` | `fn name(args) -> Type { }` |
| `block` | `block name = NMB` |
| `for` | `for int i = 0; i < N; i++ { }` |
| `while` | `while cond { }` |
| `if` | `if cond { }` |
| `ife` | `if cond { } else { }` |
| `print` | `print(...)` |
| `main` | `fn main() { }` |
| `reset` | `name.reset()` |
| `err` | `error("message")` |
| `priv` | `private` |
| `pub` | `public` |

### 3. Language Configuration

- Auto-closing pairs: `{}`, `()`, `[]`, `""`, `''`
- Bracket matching, auto-indentation
- Comment toggling (`//`)
- Indentation: 4 spaces (hardcoded — consistent with language spec)

### 4. LSP Diagnostics

The extension communicates with the Brick compiler in LSP mode:

```bash
brick input.brc --lsp
```

The compiler outputs JSON with:
- **Syntax errors**: line, column, message for each error
- **Tokens**: full token list (for semantic analysis)
- **Symbols**: function and variable declarations (for outline/navigation)

The extension shows these as inline diagnostics (red squiggles) in the editor.

### 5. Debugger Integration

The extension provides VS Code launch configurations for debugging Brick programs.

**Three debug configurations:**

| Configuration | Description |
|---------------|-------------|
| **Debug Compiler** | Debug the Brick compiler compiling the current `.brc` file |
| **Debug Compiled Program** | Compile `.brc` → C → binary with debug symbols, then launch GDB |
| **Run Compiled Program** | Compile and run (release mode, no debugger UI) |

**Setup required:**

Create `.vscode/launch.json` and `.vscode/tasks.json` using the `Brick: Init Workspace` command, or use the provided configuration snippets.

**Pre-launch tasks:**
- `Build (debug)` — builds the Brick compiler in debug mode
- `Compile this .brc file` — compiles the current `.brc` to C and links the binary
- `Compile this .brc file (release)` — same but with `-O3` and no debug symbols

**GDB integration:**

The launch configuration auto-loads Brick's debug support:

```json
{
    "setupCommands": [
        {
            "description": "Enable pretty-printing",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        },
        {
            "description": "Load Brick printers",
            "text": "source ${workspaceFolder}/debugger/.gdbinit",
            "ignoreFailures": true
        }
    ]
}
```

This gives you access to:
- Pretty-printers for `BlockCtx*` and `BrickString`
- Custom GDB commands: `info blocks`, `block <name>`, `block-watch`
- `#line` directives mapping back to `.brc` source

### 6. Memory Webview

The extension adds a "Brick Memory" view to the VS Code debug sidebar. During a debug session, this webview shows:

- All registered memory blocks
- Each block's capacity and current usage
- Visual usage bars (similar to the TUI visualizer)
- Warnings for blocks over 80% full

**How it works:**

The webview reads block information via GDB's DAP (Debug Adapter Protocol) using the custom `blocks-list` command. It parses the output and renders an HTML-based visualization.

To use the Memory Webview:

1. Start a debug session (Brick: Debug Compiled Program)
2. Set breakpoints in your `.brc` source
3. Open the "Brick Memory" view in the debug sidebar
4. Step through your code — the view updates to show block usage

---

## Development

### Project Structure

```
vscode-ext/
├── src/                    # TypeScript source
│   └── extension.ts        # Extension entry point
├── server/                 # LSP server (if separated)
├── syntaxes/
│   └── brick.tmLanguage.json  # TextMate grammar for highlighting
├── snippets/
│   └── brick.code-snippets    # Code snippets
├── out/                    # Compiled JavaScript
├── package.json            # Extension manifest
├── tsconfig.json           # TypeScript config
├── language-configuration.json  # VS Code language settings
└── install.sh              # Installation script
```

### Building

```bash
cd vscode-ext/
npm install          # Install dependencies (vscode-languageclient, @types/node, etc.)
npm run compile      # Compile TypeScript → JavaScript
npm run watch        # Watch mode for development
```

### Extension Manifest (package.json)

Key contributions defined in `package.json`:

```json
{
    "contributes": {
        "languages": [{ "id": "brick", "extensions": [".brc"] }],
        "grammars": [{ "language": "brick", "scopeName": "source.brc" }],
        "snippets": [{ "language": "brick" }],
        "views": { "debug": [{ "id": "brick.memoryView", "name": "Brick Memory", "type": "webview" }] },
        "commands": [
            { "command": "brick.initWorkspace", "title": "Brick: Init Workspace" },
            { "command": "brick.debugProgram", "title": "Brick: Debug Program" }
        ],
        "debuggers": [{ "type": "cppdbg", "label": "Brick" }]
    }
}
```

### Adding a New Snippet

Edit `snippets/brick.code-snippets`:

```json
{
    "My Snippet Name": {
        "prefix": "shortcut",
        "body": [
            "template line 1",
            "template line 2 with ${1:placeholder}"
        ],
        "description": "Description of what this snippet does"
    }
}
```

### Testing the Extension

1. Open the `vscode-ext/` folder in VS Code
2. Press `F5` to launch Extension Development Host
3. Open a `.brc` file to test highlighting and snippets
4. Test debug configurations with a simple `.brc` program

---

## Commands

Available commands (Ctrl+Shift+P → "Brick"):

| Command | Description |
|---------|-------------|
| **Brick: Init Workspace** | Creates `.vscode/launch.json` and `.vscode/tasks.json` with proper Brick debug configurations |
| **Brick: Debug Program** | Compiles and debugs the current `.brc` file in one action |

---

## Troubleshooting

### "Brick Memory" view is empty

1. Make sure you're in a debug session (not just editing)
2. Verify the program was compiled with `-DBRICK_TRACK_BLOCKS`
3. Check that GDB supports the Python commands (GDB 7.7+)
4. Try running `gdb -batch -ex "source debugger/.gdbinit" -ex "info blocks" ./program` manually

### Syntax highlighting not working

1. Ensure the file extension is `.brc`
2. Reload the window (`Ctrl+Shift+P` → `Developer: Reload Window`)
3. Check the language mode (bottom-right): should say "Brick"

### Debug launch fails

1. Make sure the Brick compiler is built: `scons` in the project root
2. Make sure tasks are configured: run "Brick: Init Workspace"
3. Check that `gdb` is available on your PATH
4. Try the "Run Compiled Program" configuration (less dependencies)

### LSP diagnostics not showing

1. The extension uses the compiler in `--lsp` mode — check that `build/brick` exists
2. Try running `build/brick your_file.brc --lsp` manually to see the JSON output
3. Check the Output panel in VS Code (View → Output → choose "Brick Language")

# Meta-C VS Code Extension

The VS Code extension provides syntax highlighting, snippets, language server protocol (LSP) integration, debugger support, and a memory block visualizer — all for the Meta-C language.

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
- **Meta-C compiler**: Built with `scons` in the project root
- **GDB**: For debugging support

---

## Features

### 1. Syntax Highlighting

Files with `.mc` extension are automatically recognized as Meta-C. The TextMate grammar provides semantic highlighting for:

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

The extension communicates with the Meta-C compiler in LSP mode:

```bash
meta-c input.mc --lsp
```

The compiler outputs JSON with:
- **Syntax errors**: line, column, message for each error
- **Tokens**: full token list (for semantic analysis)
- **Symbols**: function and variable declarations (for outline/navigation)

The extension shows these as inline diagnostics (red squiggles) in the editor.

### 5. Debugger Integration

The extension provides VS Code launch configurations for debugging Meta-C programs.

**Three debug configurations:**

| Configuration | Description |
|---------------|-------------|
| **Debug Compiler** | Debug the Meta-C compiler compiling the current `.mc` file |
| **Debug Compiled Program** | Compile `.mc` → C → binary with debug symbols, then launch GDB |
| **Run Compiled Program** | Compile and run (release mode, no debugger UI) |

**Setup required:**

Create `.vscode/launch.json` and `.vscode/tasks.json` using the `Meta-C: Init Workspace` command, or use the provided configuration snippets.

**Pre-launch tasks:**
- `Build (debug)` — builds the Meta-C compiler in debug mode
- `Compile this .mc file` — compiles the current `.mc` to C and links the binary
- `Compile this .mc file (release)` — same but with `-O3` and no debug symbols

**GDB integration:**

The launch configuration auto-loads Meta-C's debug support:

```json
{
    "setupCommands": [
        {
            "description": "Enable pretty-printing",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        },
        {
            "description": "Load Meta-C printers",
            "text": "source ${workspaceFolder}/debugger/.gdbinit",
            "ignoreFailures": true
        }
    ]
}
```

This gives you access to:
- Pretty-printers for `BlockCtx*` and `MetaCString`
- Custom GDB commands: `info blocks`, `block <name>`, `block-watch`
- `#line` directives mapping back to `.mc` source

### 6. Memory Webview

The extension adds a "Meta-C Memory" view to the VS Code debug sidebar. During a debug session, this webview shows:

- All registered memory blocks
- Each block's capacity and current usage
- Visual usage bars (similar to the TUI visualizer)
- Warnings for blocks over 80% full

**How it works:**

The webview reads block information via GDB's DAP (Debug Adapter Protocol) using the custom `blocks-list` command. It parses the output and renders an HTML-based visualization.

To use the Memory Webview:

1. Start a debug session (Meta-C: Debug Compiled Program)
2. Set breakpoints in your `.mc` source
3. Open the "Meta-C Memory" view in the debug sidebar
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
│   └── meta-c.tmLanguage.json  # TextMate grammar for highlighting
├── snippets/
│   └── meta-c.code-snippets    # Code snippets
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
        "languages": [{ "id": "meta-c", "extensions": [".mc"] }],
        "grammars": [{ "language": "meta-c", "scopeName": "source.mc" }],
        "snippets": [{ "language": "meta-c" }],
        "views": { "debug": [{ "id": "meta-c.memoryView", "name": "Meta-C Memory", "type": "webview" }] },
        "commands": [
            { "command": "meta-c.initWorkspace", "title": "Meta-C: Init Workspace" },
            { "command": "meta-c.debugProgram", "title": "Meta-C: Debug Program" }
        ],
        "debuggers": [{ "type": "cppdbg", "label": "Meta-C" }]
    }
}
```

### Adding a New Snippet

Edit `snippets/meta-c.code-snippets`:

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
3. Open a `.mc` file to test highlighting and snippets
4. Test debug configurations with a simple `.mc` program

---

## Commands

Available commands (Ctrl+Shift+P → "Meta-C"):

| Command | Description |
|---------|-------------|
| **Meta-C: Init Workspace** | Creates `.vscode/launch.json` and `.vscode/tasks.json` with proper Meta-C debug configurations |
| **Meta-C: Debug Program** | Compiles and debugs the current `.mc` file in one action |

---

## Troubleshooting

### "Meta-C Memory" view is empty

1. Make sure you're in a debug session (not just editing)
2. Verify the program was compiled with `-DMETA_C_TRACK_BLOCKS`
3. Check that GDB supports the Python commands (GDB 7.7+)
4. Try running `gdb -batch -ex "source debugger/.gdbinit" -ex "info blocks" ./program` manually

### Syntax highlighting not working

1. Ensure the file extension is `.mc`
2. Reload the window (`Ctrl+Shift+P` → `Developer: Reload Window`)
3. Check the language mode (bottom-right): should say "Meta-C"

### Debug launch fails

1. Make sure the Meta-C compiler is built: `scons` in the project root
2. Make sure tasks are configured: run "Meta-C: Init Workspace"
3. Check that `gdb` is available on your PATH
4. Try the "Run Compiled Program" configuration (less dependencies)

### LSP diagnostics not showing

1. The extension uses the compiler in `--lsp` mode — check that `build/meta-c` exists
2. Try running `build/meta-c your_file.mc --lsp` manually to see the JSON output
3. Check the Output panel in VS Code (View → Output → choose "Meta-C Language")

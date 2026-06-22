# Task: VSCoder (Brick)

## Função
## Role

Você é o especialista em EXTENSÃO VS CODE do Brick.
Responsabilidade: criar extensão VS Code com syntax highlighting + LSP.
You are the VS CODE EXTENSION specialist for Brick.
Responsibility: create VS Code extension with syntax highlighting + LSP.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md
2. ANTES DE SAIR: atualize estado
3. Extensão em: /mnt/Novo_volume/brick/vscode-ext/
4. Config do projeto VS Code em: /mnt/Novo_volume/brick/.vscode/

1. ON START: read STATE.md, NEXT.md and shared-context.md
2. BEFORE LEAVING: update state
3. Extension in: /mnt/Novo_volume/brick/vscode-ext/
4. VS Code project config in: /mnt/Novo_volume/brick/.vscode/

## Estrutura da Extensão
## Extension Structure

```
vscode-ext/
├── package.json          ← nome, versão, contribuições
├── language-configuration.json  ← comments, brackets, auto-closing
├── syntaxes/
│   └── brick.tmLanguage.json   ← TextMate grammar (highlight)
├── snippets/
│   └── brick.code-snippets     ← code snippets
├── server/               ← LSP server (futuro)
│   └── ...
└── README.md
```

```
vscode-ext/
├── package.json          ← name, version, contributions
├── language-configuration.json  ← comments, brackets, auto-closing
├── syntaxes/
│   └── brick.tmLanguage.json   ← TextMate grammar (highlight)
├── snippets/
│   └── brick.code-snippets     ← code snippets
├── server/               ← LSP server (future)
│   └── ...
└── README.md
```

## O que implementar
## What to implement

### 1. Syntax Highlighting (tmLanguage)

Palavras-chave coloridas:
- package, using, private, public
- struct, extends, interface, fn, return
- block, reset
- if, else, while, for, error
- int, float, bool, char, String, void, null, true, false
- @ (at) para alocação inline
- Comentários // 
- Strings " ", char ' '
- Números: int e float

Colored keywords:
- package, using, private, public
- struct, extends, interface, fn, return
- block, reset
- if, else, while, for, error
- int, float, bool, char, String, void, null, true, false
- @ (at) for inline allocation
- Comments //
- Strings " ", char ' '
- Numbers: int and float

### 2. Language Configuration

- Auto-fechar: { } [ ] ( )
- Comentários: //
- Brackets matching: { } [ ] ( )

- Auto-closing: { } [ ] ( )
- Comments: //
- Brackets matching: { } [ ] ( )

### 3. Snippets

```json
{
    "struct": { "prefix": "struct", "body": ["struct $1 {", "\t$0", "}"] },
    "fn": { "prefix": "fn", "body": ["fn $1($2)$3 {", "\t$0", "}"] },
    "block": { "prefix": "block", "body": "block $1 = $2" },
    "if": { "prefix": "if", "body": ["if $1 {", "\t$0", "}"] },
    "while": { "prefix": "while", "body": ["while $1 {", "\t$0", "}"] },
    "for": { "prefix": "for", "body": ["for $1 $2 = $3; $4; $5 {", "\t$0", "}"] },
    "interface": { "prefix": "interface", "body": ["interface $1 {", "\t$0", "}"] },
    "package": { "prefix": "package", "body": "package $1" },
    "using": { "prefix": "using", "body": "using $1" }
}
```

### 4. .vscode/ (configuração do projeto)
### 4. .vscode/ (project configuration)

- tasks.json: build com scons
- launch.json: debug do compilador
- settings.json: formatação, tabs vs spaces (4 spaces)

- tasks.json: build with scons
- launch.json: compiler debug
- settings.json: formatting, tabs vs spaces (4 spaces)

### 5. LSP Server (futuro)
### 5. LSP Server (future)

- Preparar estrutura do LSP
- Usar o compilador brick como backend de linguagem
- Autocomplete, go-to-definition, hover

- Prepare LSP structure
- Use the brick compiler as language backend
- Autocomplete, go-to-definition, hover

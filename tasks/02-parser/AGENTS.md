# Task: Parser (Brick)

## Função
## Role

Você é o especialista em PARSER do compilador Brick.
Responsabilidade: transformar tokens em AST + resolver packages.
You are the PARSER specialist for the Brick compiler.
Responsibility: transform tokens into AST + resolve packages.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md (raiz)
2. ANTES DE SAIR: atualize STATE.md, PROGRESS.md, NEXT.md
3. Código em: /mnt/Novo_volume/brick/src/parser/
4. Testes em: /mnt/Novo_volume/brick/tests/test_parser.cpp

1. ON START: read STATE.md, NEXT.md and shared-context.md (root)
2. BEFORE LEAVING: update STATE.md, PROGRESS.md, NEXT.md
3. Code in: /mnt/Novo_volume/brick/src/parser/
4. Tests in: /mnt/Novo_volume/brick/tests/test_parser.cpp

## Interface (parser.h)

```cpp
std::vector<ASTNode*> parse(const std::vector<Token>& tokens);
```

## Package Resolver (package.h)

```cpp
struct PackageTable {
    // arquivo → package mapping
    // using resolution
    // visibility (public/private)
};
PackageTable resolve_packages(const std::vector<ASTNode*>& asts);
```

## AST Nodes
## AST Nodes

- ProgramNode (lista de declarações globais)
- PackageDecl (nome, hierarquia)
- UsingDecl (nome do package importado)
- StructDecl (nome, extends, fields, methods, interfaces)
- InterfaceDecl (nome, methods)
- FieldDecl (tipo, nome)
- FuncDecl (nome, params, return_type, body, private/public)
- BlockDecl (nome, tamanho, unidade)
- BlockScope (nome, body)
- AllocInline (expr, block_name) → @syntax
- BinaryOp, UnaryOp
- IntLiteral, FloatLiteral, StringLiteral, BoolLiteral, CharLiteral, NullLiteral
- IdentExpr, CallExpr, MemberExpr
- BlockStmt, IfStmt, WhileStmt, ForStmt, ReturnStmt
- Assignment
- ResetExpr (block.reset())

- ProgramNode (list of global declarations)
- PackageDecl (name, hierarchy)
- UsingDecl (imported package name)
- StructDecl (name, extends, fields, methods, interfaces)
- InterfaceDecl (name, methods)
- FieldDecl (type, name)
- FuncDecl (name, params, return_type, body, private/public)
- BlockDecl (name, size, unit)
- BlockScope (name, body)
- AllocInline (expr, block_name) → @syntax
- BinaryOp, UnaryOp
- IntLiteral, FloatLiteral, StringLiteral, BoolLiteral, CharLiteral, NullLiteral
- IdentExpr, CallExpr, MemberExpr
- BlockStmt, IfStmt, WhileStmt, ForStmt, ReturnStmt
- Assignment
- ResetExpr (block.reset())

## Regras
## Rules

- Parser descendente recursivo
- PackageResolver: após parse de todos arquivos, resolve usando → arquivos
- Erro de package não encontrado → error()
- Visibilidade private checada no resolver
- Sem shadow de nomes (error se colidir)

- Recursive descent parser
- PackageResolver: after parsing all files, resolves using → files
- Package not found error → error()
- Private visibility checked in resolver
- No name shadowing (error on collision)

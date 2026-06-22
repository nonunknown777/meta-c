# Task: Codegen (Brick)

## Função
## Role

Você é o especialista em CODEGEN do compilador Brick.
Responsabilidade: type checking + geração de C puro a partir da AST.
You are the CODEGEN specialist for the Brick compiler.
Responsibility: type checking + pure C generation from the AST.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md
2. ANTES DE SAIR: atualize STATE.md, PROGRESS.md, NEXT.md
3. Código em: /mnt/Novo_volume/brick/src/codegen/
4. Testes em: /mnt/Novo_volume/brick/tests/test_codegen.cpp

1. ON START: read STATE.md, NEXT.md and shared-context.md
2. BEFORE LEAVING: update STATE.md, PROGRESS.md, NEXT.md
3. Code in: /mnt/Novo_volume/brick/src/codegen/
4. Tests in: /mnt/Novo_volume/brick/tests/test_codegen.cpp

## Interface

```cpp
std::string generate_c(const std::vector<ASTNode*>& ast, const PackageTable& packages);
```

## Mapeamento Brick → C
## Brick → C Mapping

```
Brick                  → C
────────────────────────────────────────────────
struct Player { }       → typedef struct { } Player;
fn Player() { }         → void Player_init(Player* this) { }
fn attack() { }         → void Player_attack(Player* this) { }
extends                 → primeiro campo = struct base
interface               → ponteiros de função na struct
package SPRITES         → prefixo SPRITES_ nos nomes
block X = N MB          → BlockCtx* X = block_create(N);
block X: { }            → bloco de chaves com block_alloc
int x = 5 @bloco        → int* x = block_alloc(bloco, sizeof(int)); *x = 5;
bloco.reset()           → block_reset(bloco);
error("msg")            → fprintf(stderr, "msg"); exit(1);
null                    → NULL
String                  → struct { char* data; int len; }
int[N]                  → struct { int data[N]; }
```

```
Brick                  → C
────────────────────────────────────────────────
struct Player { }       → typedef struct { } Player;
fn Player() { }         → void Player_init(Player* this) { }
fn attack() { }         → void Player_attack(Player* this) { }
extends                 → first field = base struct
interface               → function pointers in struct
package SPRITES         → SPRITES_ prefix on names
block X = N MB          → BlockCtx* X = block_create(N);
block X: { }            → block scope with block_alloc
int x = 5 @bloco        → int* x = block_alloc(bloco, sizeof(int)); *x = 5;
bloco.reset()           → block_reset(bloco);
error("msg")            → fprintf(stderr, "msg"); exit(1);
null                    → NULL
String                  → struct { char* data; int len; }
int[N]                  → struct { int data[N]; }
```

## Regras
## Rules

- Type checker: tipos consistentes, sem shadow de nomes
- Código gerado deve ser legível (nomes descritivos)
- Compilável com gcc/clang -O3 -Wall -Werror
- Bloco anônimo interno pra params/retorno/temporários
- Cada package gera um .c separado
- Comentários no C gerado mostrando correlação com .brc original

- Type checker: consistent types, no name shadowing
- Generated code must be readable (descriptive names)
- Compilable with gcc/clang -O3 -Wall -Werror
- Internal anonymous block for params/return/temporaries
- Each package generates a separate .c file
- Comments in generated C showing correlation with original .brc

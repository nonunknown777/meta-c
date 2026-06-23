# Brick Language Specification v0.4.0

## Filosofia
## Philosophy
Linguagem OOP que compila para C. Performance máxima, gerenciamento
explícito de memória por blocos, hot reload nativo. Sem stack pra dados
do usuário — tudo em blocos gerenciados.
OOP language that compiles to C. Maximum performance, explicit block-based
memory management, native hot reload. No stack for user data — everything
in managed blocks.

## Sintaxe Base
## Base Syntax
Inspirada em GDScript com chaves { } e tipos explícitos.
Indentação: 4 espaços.
Inspired by GDScript with braces { } and explicit types.
Indentation: 4 spaces.

## Packages
```brick
package SPRITES              // declara package
package SPRITES.EFFECTS      // sub-package hierárquico

using SPRITES                // importa package
using SPRITES.EFFECTS

private int internal_var     // visível só no próprio package
```

## Structs (OOP)
```brick
struct Player extends Entity {
    int hp
    String name

    fn Player(int h, String n) {    // construtor = nome da struct
        hp = h
        name = n
    }

    fn take_damage(int dmg) {
        hp -= dmg
    }
}

interface Damageable {
    fn take_damage(int d)
}
```

## Memória (Blocos)
## Memory (Blocks)
```brick
// Declaração de blocos (no arquivo main principal)
block global = 256MB
block game   = 64MB
block temp   = 8KB

// Alocação implícita (vai pro bloco global, que é o default)
int x = 5
String s = "olá"

// Escopo de bloco
block game {
    Player p = Player(100, "Felipe")
    Enemy e = Enemy(50)
}

// Alocação inline explícita
float f = 2.0 @temp
int[] arr = int[10] @game

// Reset
game.reset()     // libera TUDO no bloco game
```

### Regras de Memória
### Memory Rules
- `block nome = N KB/MB/GB` declara e aloca um bloco
- `global` é o bloco default — vars sem `@` ou `block:` vão pra ele
- `block nome:` muda o bloco default dentro do escopo
- Sem free individual — só `block.reset()` libera tudo
- Bloco cheio → `error("block overflow")` (panic)
- Ponteiros entre blocos são permitidos (refs cruzadas)
- Um bloco anônimo interno (compiler) gerencia params e retornos
- `block name = N KB/MB/GB` declares and allocates a block
- `global` is the default block — vars without `@` or `block:` go to it
- `block name:` changes the default block within scope
- No individual free — only `block.reset()` releases everything
- Full block → `error("block overflow")` (panic)
- Pointers across blocks are allowed (cross-references)
- An internal anonymous block (compiler) manages params and returns

## Tipos
## Types
```brick
// Tipos de largura fixa / Fixed-width types
i8 i16 i32 i64   // inteiros com sinal / signed integers
u8 u16 u32 u64   // inteiros sem sinal / unsigned integers
f32 f64          // ponto flutuante / floating point
usize isize      // size_t / ptrdiff_t (ponteiro / pointer-size)

// Apelidos / Aliases
int    = i32     // inteiro 32-bit (padrão) / 32-bit integer (default)
float  = f32     // ponto flutuante 32-bit (padrão) / 32-bit float (default)
char   = u8      // caractere 8-bit / 8-bit character
byte   = u8      // mesmo que char / same as char
short  = i16     // inteiro curto 16-bit / short 16-bit integer
long   = i64     // inteiro longo 64-bit / long 64-bit integer
double = f64     // ponto flutuante 64-bit / 64-bit float

bool         // true | false
String       // string built-in (dinâmica, alocada em bloco) / built-in string (dynamic, block-allocated)
T[N]         // array fixo de N elementos de tipo T / fixed array of N elements of type T
null         // ponteiro nulo / null pointer
```

### Literais com sufixo / Suffixed literals
```brick
42u8   42u16  42u32  42u64    // unsigned
42i8   42i16  42i32  42i64    // signed (42 = auto)
3.14f32  3.14f64              // float
42usz  42isize                // pointer-size
```

### Regras de tipo / Type rules
- Literal sem sufixo: tipo inferido pelo contexto (se cabe no destino, permite)
- Widening permitido (i8→i16, u8→u64, f32→f64)
- Narrowing proibido (i64→i32 = erro)
- Signed↔Unsigned mesmo rank proibido (i32↔u32 = erro)
- Int + Float → Float (int promove a float)
- Overflow em literal compile-time: erro

## Funções
## Functions
```brick
fn main() { }                // entry point

fn add(int a, int b) -> int {
    return a + b
}

fn log(String msg) {
    // params e retorno: gerenciados pelo bloco anônimo interno
    // params and return: managed by the internal anonymous block
}
```

## Controle de Fluxo
## Flow Control
```brick
if cond { }
else { }

while cond { }

for int i = 0; i < 10; i++ { }

return expr
```

## Erros
## Errors
```brick
error("msg")     // imprime e aborta (panic) / prints and aborts (panic)
```

## I/O (Package IO)
```brick
using IO

fn main() {
    print(42)                    // imprime "42\n"
    print(3.14)                  // imprime "3.140000\n"
    print(true)                  // imprime "true\n"
    print('a')                   // imprime "a\n"
    print("hello")               // imprime "hello\n"
    print()                      // imprime "\n"
    print("x = {0}", 10)         // imprime "x = 10\n"
    print("{0} + {1} = {2}", 1, 2, 3)  // imprime "1 + 2 = 3\n"
}
```

**Regras:**
- `using IO;` é obrigatório para usar `print()`
- `print()` sempre adiciona `\n` no final (println semantics)
- Tipos suportados: todos os numéricos (i8..i64, u8..u64, f32/f64, usize/isize), bool, char, String
- Formatação: `{0}`, `{1}` etc. referem-se aos argumentos posicionalmente
- Implementação: codegen gera chamadas para `runtime/io.c` (wrappers de printf com macros PRI)

**Rules:**
- `using IO;` is required to use `print()`
- `print()` always adds `\n` at the end (println semantics)
- Supported types: all numeric (i8..i64, u8..u64, f32/f64, usize/isize), bool, char, String
- Formatting: `{0}`, `{1}` etc. refer to arguments positionally
- Implementation: codegen generates calls to `runtime/io.c` (printf wrappers with PRI macros)

## C Interop (extern / include / link)
```brick
// Incluir header C e linkar biblioteca
include "math.h" and link m

// Declarar função C externa
extern fn sqrt(f64 x) -> f64       // 1:1 com double sqrt(double)
extern fn atoi(*u8 str) -> i32     // *u8 = char*, String vira .data
extern fn puts(*u8 s) -> i32       // String → char* automático
extern fn sin(f64 x) -> f64
extern fn cos(f64 x) -> f64
extern fn pow(f64 b, f64 exp) -> f64
extern fn ceil(f64 x) -> f64
extern fn floor(f64 x) -> f64

// Separar include e link
include "SDL.h"
link SDL2

// Apenas link sem header
link pthread
extern fn pthread_create(...) -> i32

// Usando em código Brick
using IO

fn main() {
    include "math.h" and link m
    double r = sqrt(2.0)

    String msg = "C interop works!"
    puts(msg)           // msg.data passado automaticamente

    i32 n = atoi("42")  // "42".data → char*
}
```

**Regras:**
- `include "header.h"` — string literal com path do header C (entre aspas)
- `link libname` — identifier com nome da lib (sem `"`, sem prefixo `-l`)
- `include "h" and link lib` — sintaxe combinada (conectivo `and`)
- `extern fn nome(params) -> ret` — declara função C para o type checker
- `*T` — prefixo `*` define ponteiro: `*u8` = `char*`, `*void` = `void*`, `*MyStruct` = `MyStruct*`
- `String` → `*u8` — conversão automática de `.data` ao passar String pra parâmetro `*u8`
- Protótipos C NÃO são gerados pelo codegen — os headers incluídos fornecem as declarações
- Para funções de headers sempre inclusos (stdio.h, stdlib.h, string.h), o `extern fn` só serve pro type checker
- `brick bind <header.h>` — comando CLI que gera binding .brc de um header C (regex simples)

**C Interop (extern / include / link)**
- `include "header.h"` — string literal with C header path (with quotes)
- `link libname` — identifier with lib name (no `"`, no `-l` prefix)
- `include "h" and link lib` — combined syntax (`and` connective)
- `extern fn name(params) -> ret` — declares C function for the type checker
- `*T` — `*` prefix defines pointer: `*u8` = `char*`, `*void` = `void*`, `*MyStruct` = `MyStruct*`
- `String` → `*u8` — automatic `.data` conversion when passing String to `*u8` parameter
- C prototypes are NOT generated by codegen — included headers provide declarations
- For functions from always-included headers (stdio.h, stdlib.h, string.h), `extern fn` only serves the type checker
- `brick bind <header.h>` — CLI command to generate .brc bindings from a C header (simple regex)

## Compilação
## Compilation
```bash
brick input.brc -o output.c    # gera C (com #line directives)
gcc -O3 output.c runtime/block_memory.c runtime/io.c -o programa
gcc -g output.c runtime/block_memory.c runtime/hot_reload.c runtime/io.c -o programa   # debug
```

## Debugging
- `#line` directives no código C gerado mapeiam .brc → .c
- GDB mostra código-fonte Brick original, não o C gerado
- GDB pretty-printers para BlockCtx (info blocks, block name)
- VS Code webview com visualização gráfica dos blocos de memória
- Comandos GDB custom: `info blocks`, `block <nome>`, `block-watch`
- `#line` directives in the generated C code map .brc → .c
- GDB shows the original Brick source, not the generated C
- GDB pretty-printers for BlockCtx (info blocks, block name)
- VS Code webview with graphical visualization of memory blocks
- Custom GDB commands: `info blocks`, `block <name>`, `block-watch`

## Hot Reload
Programas compilados com Brick suportam hot reload via dlopen.
Cada package compila para um .so separado.
Monitoramento via inotify. Swap atômico de ponteiros de função.
Programs compiled with Brick support hot reload via dlopen.
Each package compiles to a separate .so.
Monitoring via inotify. Atomic swap of function pointers.

## Arquitetura do Compilador
## Compiler Architecture
```
.brc file
  → Lexer (tokens)
  → Parser (AST + package resolution)
  → Codegen (type check + geração C com #line)
  → output.c + runtime.c → gcc -O3 → binário
  ↑
  └── Tester/Optimizer (task 10) testa, otimiza e documenta tudo
```

```
.brc file
  → Lexer (tokens)
  → Parser (AST + package resolution)
  → [Macro System (collect + eval build + expand)]
  → Codegen (type check + C generation with #line)
  → output.c + runtime.c → gcc -O3 → binary
  ↑
  └── Tester/Optimizer (task 10) tests, optimizes and documents everything
```

## Tester/Optimizer (Task 10)
Task sênior do projeto. Responsável por:
- Executar testes unitários e de integração
- Otimizar performance do compilador, runtime e código gerado
- Documentar código inline (comentários simples) + docs .md
- Atualizar STATE.md das tasks 01-09 com progresso e descobertas
- Verificar consistência entre interfaces (TokenType, AST, etc)
Senior task of the project. Responsible for:
- Running unit and integration tests
- Optimizing performance of compiler, runtime and generated code
- Documenting inline code (simple comments) + docs .md
- Updating STATE.md of tasks 01-09 with progress and findings
- Checking consistency between interfaces (TokenType, AST, etc)

## Macro System (v0.5.0)

Macros geram código em tempo de compilação.

### 1. `macro` — Template de Código
```brick
macro nome(param1, param2, valores...) {
    $param1 = $param2 + 1
    emit { fn $param1() { } }
}
```
- Parâmetros não têm tipo (seguram expressões)
- `$nome` insere o argumento
- `$(expr)` avalia expr em tempo de compilação e insere resultado
- `valores...` captura argumentos restantes como lista indexável

### 2. `build` — Computação em Tempo de Compilação
```brick
build {
    x = 42
    emit { z = x + 10 }   // z = 52 no código final
}
```
- Suporta: aritmética, for/while/if, atribuição, strings
- `emit { }` gera código no local do build
- `emit nome(args)` — atalho para chamar macro
- Type reflection: `T.name`, `T.size`, `T.fields`

### 3. `emit` — Geração de Código
```brick
emit { /* qualquer código Brick */ }
emit nome_macro(args)
```

### Pipeline
1. **Collect**: `macro` declarations → tabela (removidas da AST)
2. **Eval Build**: `build {}` executa em tempo de compilação, `emit` gera código
3. **Expand**: `macro_call(...)` → substituição `$` + gensym `__` + inserção do corpo
4. **Clean**: AST final sem macros → type checker → codegen

### Higiene
- Variáveis `__` dentro de macros ganham sufixo `__1`, `__2`, etc.
- Parâmetros do usuário NÃO são renomeados
- Gensym counter global por unidade de compilação

### Limites
- Recursão máxima: 64 níveis (detectado como erro)
- `$` fora de macro/build: erro de compilação
- I/O bloqueado dentro de `build`

# Meta-C Language Reference

This is the complete reference for the Meta-C programming language. Everything you need to know to write `.mc` files.

---

## File Structure

A Meta-C source file (`.mc`) follows this structure:

```meta-c
package NOME               // which package this file belongs to
using OUTRO                // import another package

block global = 256MB       // REQUIRED: block declarations at the top
block game = 64MB

struct Player { }          // struct definitions with methods

fn main() { }              // entry point (required)
```

## Packages

Packages organize code into namespaces. Every file must declare its package.

```meta-c
package SPRITES                // declares the "SPRITES" package
package SPRITES.EFFECTS        // hierarchical sub-package

using SPRITES                  // import everything from SPRITES
using SPRITES.EFFECTS          // import sub-package

private int internal_var       // visible only within the package
```

- Everything is `public` by default
- `private` restricts visibility to the current package
- `using` makes exported symbols available in the current file
- Packages are hierarchical (separated by dots)

## Comments

Only single-line comments are supported:

```meta-c
// This is a comment — goes until end of line
int x = 5  // inline comment
```

Block comments (`/* */`) are **not** supported.

## Types

### Primitive Types

| Type | Description | Size | Range |
|------|-------------|:----:|-------|
| `int` | Integer number | 64-bit | -9.2e18 to 9.2e18 |
| `float` | Decimal number | 64-bit | Double precision (IEEE 754) |
| `bool` | Boolean (true/false) | 8-bit | 0 or 1 |
| `char` | Single character | 8-bit | ASCII |
| `String` | Dynamic text | dynamic | Heap-allocated in a block |
| `void` | Nothing (for functions) | — | — |
| `null` | Null pointer literal | — | — |

### Array Types

```meta-c
int[10] arr              // fixed array of 10 integers
float[5] values          // fixed array of 5 floats
String[100] names        // array of 100 Strings
```

Array size is fixed at declaration and cannot be changed.

### Type Mapping to C

When compiled, Meta-C types map to C types as follows:

| Meta-C | C |
|--------|---|
| `int` | `int64_t` |
| `float` | `double` |
| `bool` | `uint8_t` |
| `char` | `char` |
| `String` | `MetaCString` (struct with `data` and `len`) |
| `block` | `BlockCtx*` |

## Blocks of Memory

Blocks are the foundation of Meta-C's memory model. You declare them at the file level:

```meta-c
block global = 256MB       // default block — variables go here by default
block game   = 64MB
block temp   = 8KB
block data   = 1GB
```

**Units**: `KB`, `MB`, `GB` (case-sensitive).

### Allocation Modes

**1. Default block** — variables without `@` or `block:` go to `global`:
```meta-c
int x = 5                  // allocated in global (the default block)
String s = "hello"         // also in global
```

**2. Block scope** — everything inside a `block name {}` block targets that block:
```meta-c
block game {
    Player p = Player(100, "Felipe")   // both allocated in 'game'
    Enemy e = Enemy(50)
}
```

**3. Inline annotation** — allocate a specific variable in a specific block:
```meta-c
float f = 2.0 @temp        // f lives in 'temp' block
int[] arr = int[10] @game  // arr lives in 'game' block
```

### Block Operations

```meta-c
game.reset()               // release ALL memory in 'game' — O(1), super fast
global.reset()             // release everything in 'global'
```

- **No individual free**: You can only reset entire blocks.
- **Cross-references**: Pointers between blocks are allowed.
- **Overflow**: If a block fills up, the program panics with `error("block overflow")`.

### The Anonymous Block

The compiler manages an internal anonymous block for function parameters and return values. You don't need to worry about it — it's automatic.

See [Memory Blocks](Memory-Blocks) for the complete deep dive.

## Structs (OOP)

Structs group data with methods. They are the core OOP mechanism.

```meta-c
struct Player {
    int hp
    String name
    int ammo

    // Constructor — same name as the struct
    fn Player(int h, String n, int a) {
        hp = h
        name = n
        ammo = a
    }

    // Method
    fn take_damage(int dmg) {
        hp -= dmg
    }

    // Method with return value
    fn get_hp() -> int {
        return hp
    }
}
```

### Constructor

A constructor is a function with the same name as the struct. It is called when you create an instance:

```meta-c
Player p = Player(100, "Felipe", 30) @game
```

### Inheritance

```meta-c
struct NPC extends Player {
    int ai_type

    fn NPC(int h, String n, int a, int ai) {
        hp = h          // inherited from Player
        name = n        // inherited
        ammo = a        // inherited
        ai_type = ai    // new field
    }

    fn patrol() {
        // NPC-specific behavior
    }
}
```

Inherited fields are directly accessible in the child struct. The generated C code places the parent struct as the first field (`base`), ensuring memory layout compatibility.

### Interfaces

```meta-c
interface Damageable {
    fn take_damage(int d)
}

interface Serializable {
    fn save() -> String
}

// A struct can implement multiple interfaces
struct Enemy : Damageable, Serializable {
    int hp

    fn take_damage(int d) {
        hp -= d
    }

    fn save() -> String {
        return "Enemy:" + hp
    }
}
```

### Visibility

```meta-c
public int x               // visible everywhere (default)
private int y              // visible only within the package
```

- Struct fields default to `public`
- Methods default to `public`
- `private` fields/methods are only accessible from the same package

### No `this`, No Name Shadowing

Meta-C does **not** use `this`. Inside a method, you can reference fields directly:

```meta-c
fn take_damage(int dmg) {
    hp -= dmg              // 'hp' is the struct field, not a parameter
}
```

Name shadowing is **not allowed** — you cannot have a parameter or local variable with the same name as a struct field.

## Functions

Functions are declared with `fn`:

```meta-c
fn main() { }                            // entry point (returns void)

fn add(int a, int b) -> int {            // returns int
    return a + b
}

fn log(String msg) {                     // returns void (no arrow)
    // ...
}

fn multiply(int a, int b) -> int {
    return a * b
}
```

- `fn main()` is the program entry point — always required
- Return type is specified with `-> Type`
- No `->` means `void` (no return value)
- Parameters and return values are managed by the internal anonymous block
- Function names must be unique within a package (overloading is not supported yet)

### Calling Functions

```meta-c
int result = add(3, 4)
log("calculation complete")
```

### Calling Methods

```meta-c
Player p = Player(100, "Felipe", 30) @game
p.take_damage(20)
int current_hp = p.get_hp()
```

## Control Flow

### If/Else

```meta-c
if hp <= 0 {
    print("dead")
} else {
    print("alive")
}

// Nested
if x > 0 {
    if x > 100 {
        print("large")
    }
}
```

### While

```meta-c
while hp > 0 {
    apply_damage(10)
    hp -= 10
}
```

### For

```meta-c
for int i = 0; i < 10; i++ {
    print(i)
}
```

### Return

```meta-c
fn add(int a, int b) -> int {
    return a + b
}

fn log(String msg) {
    // no return needed for void
}
```

## Operators

### Arithmetic

| Operator | Description |
|:--------:|-------------|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |

### Comparison

| Operator | Description |
|:--------:|-------------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |

### Logical

| Operator | Description |
|:--------:|-------------|
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `!` | Logical NOT |

### Bitwise

| Operator | Description |
|:--------:|-------------|
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `<<` | Left shift |
| `>>` | Right shift |

### Assignment

| Operator | Description |
|:--------:|-------------|
| `=` | Assign |
| `+=` | Add and assign |
| `-=` | Subtract and assign |
| `*=` | Multiply and assign |
| `/=` | Divide and assign |

### Other

| Operator | Description |
|:--------:|-------------|
| `.` | Access field or method |
| `()` | Function/method call |
| `[]` | Array index |
| `@` | Allocate in specific block |
| `->` | Return type annotation |

## Strings

```meta-c
String s = "hello"                   // creates a String
String name = "Felipe" @game         // String in a specific block
String empty = ""                    // empty string
```

- `String` is a built-in type with two fields: `data` (pointer to characters) and `len` (length)
- Strings are block-allocated like any other data
- Escape sequences: `\n` (newline), `\t` (tab), `\\` (backslash), `\"` (quote)

## Arrays

```meta-c
int[10] arr                          // fixed array of 10 integers
int[5] vals = int[5] @game           // array in a specific block
float[100] data                      // array of 100 floats
```

- Array size is fixed at declaration
- Indexing: `arr[0]`, `arr[i]`
- Arrays are block-allocated like any other value

## I/O (Print)

The `IO` package provides `print()`:

```meta-c
using IO

fn main() {
    print(42)                    // prints "42\n"
    print(3.14)                  // prints "3.140000\n"
    print(true)                  // prints "true\n"
    print('a')                   // prints "a\n"
    print("hello")               // prints "hello\n"
    print()                      // prints "\n"
    print("x = {0}", 10)         // prints "x = 10\n"
    print("{0} + {1} = {2}", 1, 2, 3)  // prints "1 + 2 = 3\n"
}
```

Rules:
- `using IO;` is required to use `print()`
- `print()` always adds `\n` (println semantics)
- Supported types: `int`, `float`, `bool`, `char`, `String`
- Formatting uses `{0}`, `{1}`, etc. referencing positional arguments
- Internally generates calls to `runtime/io.c` (printf wrappers)

## Error Handling

```meta-c
error("something went wrong")    // prints message and aborts (panic)
```

- There is **no try/catch** or exception mechanism
- When `error()` is called, the program prints the message and exits immediately
- This keeps performance high and code simple
- Use it for unrecoverable conditions (block overflow, assertion failures)

## Reserved Keywords

```
package   using     public    private   struct
extends   interface fn        return    if
else      while     for       block     reset
true      false     null      error     int
float     bool      char      String    void
```

## Complete Example

```meta-c
package JOGO

using SPRITES
using IO

block global = 256MB
block game = 64MB
block temp = 8MB

struct Player {
    int hp
    int ammo
    String name

    fn Player(int h, int a, String n) {
        hp = h
        ammo = a
        name = n
    }

    fn shoot(Enemy e) {
        e.hp -= 10
    }

    fn is_alive() -> bool {
        return hp > 0
    }
}

struct Enemy {
    int hp
    int damage

    fn Enemy(int h, int d) {
        hp = h
        damage = d
    }
}

fn main() {
    Player p = Player(100, 30, "Felipe") @game
    Enemy e = Enemy(50, 10) @game

    p.shoot(e)

    while e.hp > 0 {
        p.shoot(e)
    }

    print("Player alive: {0}", p.is_alive())

    game.reset()
    global.reset()
}
```

---

## Compilation

```bash
# Step 1: Meta-C compiler produces C code
meta-c input.mc -o output.c

# Step 2: gcc compiles C + runtime into a binary
gcc -O3 output.c runtime/block_memory.c runtime/io.c -o program -ldl

# Debug build (with GDB support)
gcc -g output.c runtime/block_memory.c runtime/hot_reload.c runtime/io.c -o program -ldl
```

See [Getting Started](Getting-Started) for the full setup guide.

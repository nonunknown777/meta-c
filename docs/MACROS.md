# Brick Macros

Macros let you generate code at compile time. Think of them as "smart templates" — you write the pattern once, and the compiler stamps it out wherever you call it.

> **Zero cost.** Macros expand before type-checking. The generated code is exactly as fast as if you'd written it by hand.

---

## Quick Example: Swap

```brick
macro swap(a, b) {
    __tmp = $a
    $a = $b
    $b = __tmp
}

fn main() {
    x = 10
    y = 20
    swap(x, y)
    print("x={0} y={1}", x, y)   // x=20 y=10
}
```

The `macro swap(a, b)` defines a pattern. `$a` and `$b` are replaced with the actual variables you pass. `__tmp` gets a unique name so it never conflicts with other variables.

---

## Defining a Macro

```brick
macro name(param1, param2, ...) {
    // body — any Brick code with $ interpolation
}
```

- Parameters are **untyped** — they hold expressions, not values
- `$name` inserts the expression passed for `name`
- `$(expr)` evaluates `expr` and inserts the result (for computed names)

---

## Interpolation (`$`)

| Syntax | What it does | Example |
|--------|-------------|---------|
| `$name` | Inserts the argument passed for `name` | `$a` → `x` |
| `$(expr)` | Evaluates `expr` at compile time, inserts result | `$("vec2_" ++ name)` → `vec2_position` |

---

## Varargs (`...`)

```brick
macro print_all(values...) {
    $values[0]    // first value
    $values[1]    // second value
    // ...
}
```

`values...` captures **all remaining arguments** as a list. Access them by index.

---

## `emit` — Generate Code

```brick
macro vec2_add(name) {
    emit {
        fn $name(x1, y1, x2, y2, out_x, out_y) {
            out_x = x1 + x2
            out_y = y1 + y2
        }
    }
}

vec2_add(add_positions)

fn main() {
    add_positions(1, 2, 3, 4, result_x, result_y)
}
```

`emit { ... }` stamps its contents into the output at the call site. Everything inside `emit` is literal code with `$` interpolation.

---

## `build` — Compile-Time Computation

```brick
build {
    x = 42
    y = x + 10
    emit { z = y }
}
```

`build` runs at **compile time**. Variables created inside `build` don't exist in the final binary — only the `emit` calls produce actual code.

### Real Example: Compile-Time Damage Table

```brick
build {
    // Pre-compute damage values for all weapon types
    damages = [10, 25, 50, 100]    // fist, pistol, rifle, rocket
    max_hp = 100
    
    for i = 0; i < 4; i = i + 1 {
        dmg = damages[i]
        hp_after = max_hp - dmg
        
        emit {
            if weapon_type == i {
                hp = hp - dmg
                print("Hit! HP left: {0}", hp_after)
            }
        }
    }
}
```

This generates 4 `if` branches — all computed at compile time, zero runtime overhead.

---

## Real Game Examples

### 1. Assert Macro (Debug Checking)

```brick
macro assert(condition, message) {
    if !($condition) {
        print("ASSERT FAILED: {0}", $message)
    }
}

fn take_damage(hp, dmg) {
    assert(dmg >= 0, "damage can't be negative")
    assert(hp > 0, "already dead")
    hp = hp - dmg
}
```

### 2. Enum Pattern (Named Constants)

```brick
macro enum(name, values...) {
    emit {
        $name = 0     // first value = 0
    }
    for i = 1; i < 4; i = i + 1 {
        emit {
            ${"$" ++ values[i]} = i
        }
    }
}

// Usage:
enum(WeaponType, WEAPON_FIST, WEAPON_PISTOL, WEAPON_RIFLE, WEAPON_ROCKET)

fn main() {
    if current_weapon == WEAPON_ROCKET {
        print("Fire in the hole!")
    }
}
```

### 3. Property Getter/Setter

```brick
macro property(name, type) {
    __hidden_$name = 0
    
    emit {
        fn get_$name() {
            return __hidden_$name
        }
        fn set_$name(val) {
            __hidden_$name = val
        }
    }
}

struct Player {
    property(hp, i32)
    property(mana, i32)
    
    fn take_damage(dmg) {
        set_hp(get_hp() - dmg)
    }
}
```

### 4. Saveable Struct Serialization

```brick
macro saveable(name, fields...) {
    emit {
        fn save_$name(obj, file) {
            print(file, $name)
        }
        fn load_$name(file) -> $name {
            result = $name()
            print("loaded")
            return result
        }
    }
}

struct GameState {
    i32 level
    f64 health
    String checkpoint
    
    saveable(GameState, level, health, checkpoint)
}
```

### 5. Block Pool (Typed Allocator)

```brick
macro block_pool(name, type, count) {
    block $name = count * 64   // reserve memory
    
    emit {
        fn alloc_$name() -> $type {
            return $type() @$name
        }
        fn reset_$name() {
            reset $name
        }
    }
}

block_pool(enemies, Enemy, 100)

fn spawn_enemy(x, y) {
    e = alloc_enemies()
    e.x = x
    e.y = y
    return e
}
```

### 6. Compile-Time Constants

```brick
build {
    max_players = 4
    tile_size = 16
    screen_w = 800
    screen_h = 600
    
    emit {
        PLAYER_SPEED = 3.0f64 * tile_size / 60.0f64
        BULLET_SPEED = 10.0f64 * tile_size / 60.0f64
    }
}
```

---

## Hygiene (Name Conflicts)

Variables starting with `__` inside a macro get **unique names** automatically:

```brick
macro twice(body) {
    __i = 0
    while __i < 2 {
        $body
        __i = __i + 1
    }
}

fn main() {
    __i = 100       // ← this is a DIFFERENT variable
    twice(print("hi"))
    print(__i)      // still 100 — macro used __i__1 internally
}
```

The macro's `__i` becomes `__i__1` (or `__i__2`, etc.) — no collision.

---

## Type Reflection (inside `build`)

Inside `build` blocks you can inspect types at compile time:

| Expression | Returns | Example |
|-----------|---------|---------|
| `T.name` | Type name as string | `"i32"` |
| `T.size` | Size in bytes | `4` |
| `T.fields` | Field names as strings | `["x", "y"]` |

```brick
struct Vec2 { f32 x; f32 y }

build {
    v = Vec2
    name = v.name       // "Vec2"
    size = v.size       // 8
    fields = v.fields   // ["x", "y"]
    
    emit {
        print("Type {0} has {1} fields: {2}, {3}", name, fields.len, fields[0], fields[1])
    }
}
```

---

## Error Handling

| Situation | What happens |
|-----------|-------------|
| Wrong argument count | Compile error: "macro 'name' expects N args, got M" |
| `$` outside macro/build | Compile error: "unexpected $" |
| Recursive macro | Caught after 64 levels: "macro recursion too deep" |
| I/O inside `build` | Build blocks can't call print() or file operations |

---

## How It Works (Pipeline)

```
.brc → Lexer → Parser → [Collect Macros] → [Eval Build] → [Expand Macros] → Type Checker → Codegen → .c
```

1. **Collect**: All `macro` declarations are gathered into a table (and removed from the AST)
2. **Eval Build**: `build {}` blocks run at compile time — their `emit` calls produce generated code
3. **Expand**: Each `macro_call(...)` is replaced by the macro's body with `$` substitutions and `__` renaming
4. **Clean**: Only regular Brick code reaches the type checker and codegen

---

## See Also

- [Language Reference](LANGUAGE.md) — full syntax and type system
- [Getting Started](GETTING_STARTED.md) — install and first project
- [Architecture](ARCHITECTURE.md) — how the compiler pipeline works

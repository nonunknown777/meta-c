# Macros no Brick

Macros geram código em tempo de compilação. Pense nelas como "modelos inteligentes" — você escreve o padrão uma vez, e o compilador replica onde você chamar.

> **Custo zero.** Macros expandem antes da verificação de tipos. O código gerado é tão rápido quanto se você tivesse escrito à mão.

---

## Exemplo Rápido: Swap (Troca)

```brick
macro troca(a, b) {
    __temp = $a
    $a = $b
    $b = __temp
}

fn main() {
    x = 10
    y = 20
    troca(x, y)
    print("x={0} y={1}", x, y)   // x=20 y=10
}
```

O `macro troca(a, b)` define um padrão. `$a` e `$b` são substituídos pelas variáveis reais que você passar. `__temp` ganha um nome único pra nunca conflitar com outras variáveis.

---

## Definindo uma Macro

```brick
macro nome(param1, param2, ...) {
    // corpo — qualquer código Brick com interpolação $
}
```

- Parâmetros **não têm tipo** — eles seguram expressões, não valores
- `$nome` insere a expressão passada para `nome`
- `$(expr)` avalia `expr` em tempo de compilação e insere o resultado

---

## Interpolação (`$`)

| Sintaxe | O que faz | Exemplo |
|---------|-----------|---------|
| `$nome` | Insere o argumento passado para `nome` | `$a` → `x` |
| `$(expr)` | Avalia `expr` em tempo de compilação, insere resultado | `$("vec2_" ++ nome)` → `vec2_posicao` |

---

## Varargs (`...`)

```brick
macro print_tudo(valores...) {
    $valores[0]    // primeiro valor
    $valores[1]    // segundo valor
    // ...
}
```

`valores...` captura **todos os argumentos restantes** como uma lista. Acesse por índice.

---

## `emit` — Gerar Código

```brick
macro vec2_adic(nome) {
    emit {
        fn $nome(x1, y1, x2, y2, saida_x, saida_y) {
            saida_x = x1 + x2
            saida_y = y1 + y2
        }
    }
}

vec2_adic(soma_posicoes)

fn main() {
    soma_posicoes(1, 2, 3, 4, res_x, res_y)
}
```

`emit { ... }` carimba seu conteúdo na saída no local da chamada. Tudo dentro de `emit` é código literal com interpolação `$`.

---

## `build` — Computação em Tempo de Compilação

```brick
build {
    x = 42
    y = x + 10
    emit { z = y }
}
```

`build` roda em **tempo de compilação**. Variáveis criadas dentro de `build` não existem no binário final — só as chamadas `emit` produzem código real.

### Exemplo Real: Tabela de Dano em Tempo de Compilação

```brick
build {
    // Pré-calcula danos para todos os tipos de arma
    danos = [10, 25, 50, 100]    // soco, pistola, rifle, foguete
    hp_max = 100
    
    for i = 0; i < 4; i = i + 1 {
        dmg = danos[i]
        hp_rest = hp_max - dmg
        
        emit {
            if tipo_arma == i {
                hp = hp - dmg
                print("Acertou! HP restante: {0}", hp_rest)
            }
        }
    }
}
```

Isso gera 4 blocos `if` — todos computados em tempo de compilação, zero custo em runtime.

---

## Exemplos Reais de Jogos

### 1. Macro Assert (Verificação de Debug)

```brick
macro assert(condicao, mensagem) {
    if !($condicao) {
        print("ASSERT FALHOU: {0}", $mensagem)
    }
}

fn tomar_dano(hp, dmg) {
    assert(dmg >= 0, "dano não pode ser negativo")
    assert(hp > 0, "já está morto")
    hp = hp - dmg
}
```

### 2. Padrão Enum (Constantes Nomeadas)

```brick
macro enum(nome, valores...) {
    emit {
        $nome = 0     // primeiro valor = 0
    }
    for i = 1; i < 4; i = i + 1 {
        emit {
            ${"$" ++ valores[i]} = i
        }
    }
}

// Uso:
enum(TipoArma, ARMA_SOCO, ARMA_PISTOLA, ARMA_RIFLE, ARMA_FOGUETE)

fn main() {
    if arma_atual == ARMA_FOGUETE {
        print("Fogo no buraco!")
    }
}
```

### 3. Propriedades (Getter/Setter)

```brick
macro propriedade(nome, tipo) {
    __escondido_$nome = 0
    
    emit {
        fn get_$nome() {
            return __escondido_$nome
        }
        fn set_$nome(val) {
            __escondido_$nome = val
        }
    }
}

struct Jogador {
    propriedade(hp, i32)
    propriedade(mana, i32)
    
    fn tomar_dano(dmg) {
        set_hp(get_hp() - dmg)
    }
}
```

### 4. Serialização de Struct Salvavel

```brick
macro salvar(nome, campos...) {
    emit {
        fn salvar_$nome(obj, arquivo) {
            print(arquivo, $nome)
        }
        fn carregar_$nome(arquivo) -> $nome {
            resultado = $nome()
            print("carregado")
            return resultado
        }
    }
}

struct EstadoJogo {
    i32 fase
    f64 vida
    String checkpoint
    
    salvar(EstadoJogo, fase, vida, checkpoint)
}
```

### 5. Pool de Blocos (Alocador Tipado)

```brick
macro pool_bloco(nome, tipo, quantidade) {
    block $nome = quantidade * 64   // reserva memória
    
    emit {
        fn alocar_$nome() -> $tipo {
            return $tipo() @$nome
        }
        fn resetar_$nome() {
            reset $nome
        }
    }
}

pool_bloco(inimigos, Inimigo, 100)

fn spawnar_inimigo(x, y) {
    e = alocar_inimigos()
    e.x = x
    e.y = y
    return e
}
```

### 6. Constantes em Tempo de Compilação

```brick
build {
    max_jogadores = 4
    tamanho_tile = 16
    largura_tela = 800
    altura_tela = 600
    
    emit {
        VELOCIDADE_JOGADOR = 3.0f64 * tamanho_tile / 60.0f64
        VELOCIDADE_TIRO = 10.0f64 * tamanho_tile / 60.0f64
    }
}
```

---

## Higiene (Evitar Conflitos de Nome)

Variáveis começando com `__` dentro de uma macro ganham **nomes únicos** automaticamente:

```brick
macro duas_vezes(corpo) {
    __i = 0
    while __i < 2 {
        $corpo
        __i = __i + 1
    }
}

fn main() {
    __i = 100       // ← esta é uma variável DIFERENTE
    duas_vezes(print("oi"))
    print(__i)      // ainda 100 — a macro usou __i__1 internamente
}
```

O `__i` da macro vira `__i__1` (ou `__i__2`, etc.) — sem colisão.

---

## Reflexão de Tipos (dentro de `build`)

Dentro de blocos `build` você pode inspecionar tipos em tempo de compilação:

| Expressão | Retorna | Exemplo |
|-----------|---------|---------|
| `T.nome` | Nome do tipo como string | `"i32"` |
| `T.tamanho` | Tamanho em bytes | `4` |
| `T.campos` | Nomes dos campos como strings | `["x", "y"]` |

```brick
struct Vec2 { f32 x; f32 y }

build {
    v = Vec2
    nome = v.nome       // "Vec2"
    tamanho = v.tamanho // 8
    campos = v.campos   // ["x", "y"]
    
    emit {
        print("Tipo {0} tem {1} campos: {2}, {3}", nome, campos.len, campos[0], campos[1])
    }
}
```

---

## Tratamento de Erros

| Situação | O que acontece |
|----------|---------------|
| Contagem errada de argumentos | Erro de compilação: "macro 'nome' esperava N args, recebeu M" |
| `$` fora de macro/build | Erro de compilação: "$ inesperado" |
| Macro recursiva | Capturado após 64 níveis: "recursão de macro muito profunda" |
| I/O dentro de `build` | Blocos build não podem chamar print() ou operações de arquivo |

---

## Como Funciona (Pipeline)

```
.brc → Lexer → Parser → [Coletar Macros] → [Avaliar Build] → [Expandir Macros] → Verificador de Tipos → Gerador C → .c
```

1. **Coletar**: Todas as declarações `macro` são reunidas numa tabela (e removidas da AST)
2. **Avaliar Build**: Blocos `build {}` rodam em tempo de compilação — seus `emit` produzem código gerado
3. **Expandir**: Cada `chamada_macro(...)` é substituída pelo corpo da macro com substituições `$` e renomeação `__`
4. **Limpar**: Só código Brick normal chega ao verificador de tipos e gerador de C

---

## Veja Também

- [Referência da Linguagem](LANGUAGE.pt-BR.md) — sintaxe completa e sistema de tipos
- [Primeiros Passos](GETTING_STARTED.pt-BR.md) — instalação e primeiro projeto
- [Arquitetura](ARCHITECTURE.pt-BR.md) — como o pipeline do compilador funciona

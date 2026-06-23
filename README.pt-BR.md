<p align="center">
  <img src="docs/logo.png" alt="Brick Logo" width="200"/>
</p>

<h1 align="center">Brick</h1>
<p align="center">
  <em>Uma linguagem OOP de alta performance que compila para C puro.</em>
</p>

<p align="center">
  <a href="README.md">🇬🇧 English</a>
</p>

---

## 👋 Demonstração Rápida

```brick
package DEMO

using IO

block global = 64MB

interface Damageable {
    fn take_damage(i32 dmg)
}

struct Player {
    i32 hp
    String name
    i32 ammo

    fn Player(i32 h, String n, i32 a) {
        hp = h
        ammo = a
        name = n
    }

    fn take_damage(i32 dmg) {
        hp -= dmg
        print("{0} tomou {1} de dano, hp={2}", name, dmg, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe", 30)
    p.take_damage(20)
}
```

## Compilar & Executar

```bash
brick run exemplo.brc
```

Sem `gcc` manual — `brick build` e `brick run` cuidam de tudo.

---

## ✨ Funcionalidades

- **OOP com Chaves** — `struct` com construtores, métodos, herança e interfaces.
- **Compila para C Puro** — Código C legível com diretivas `#line` para debug. Sem VM, sem interpretador — código nativo.
- **Memória por Blocos** — Sem `malloc`/`free`, sem GC. Declare blocos (`block nome = 64MB`) e o bump allocator cuida do resto.
- **Sem Pilha para Dados** — Tudo vive em blocos gerenciados. Reset de bloco recupera tudo instantaneamente.
- **Hot Reload Nativo** — Troque código sem parar o programa via `dlopen` + `inotify`.
- **Visualizador TUI** — Dashboard ncurses mostrando estado dos blocos em tempo real.
- **Integração GDB** — Debug no código `.brc` original com pretty-printers para `BlockCtx`.
- **Extensão VS Code** — Syntax highlighting, LSP e webview de memória.
- **Tipos de Largura Fixa** — `i8/i16/i32/i64`, `u8/u16/u32/u64`, `f32/f64`, `usize`/`isize`.

---

## 🚀 Comece Agora

### Pré-requisitos

- Linux (ou Windows com mingw-w64)
- Compilador C++20 (GCC ≥ 11 ou Clang ≥ 14)
- SCons (`pip install scons`)
- ncurses (opcional, para o visualizador)

### Build do Compilador

```bash
git clone https://github.com/nonunknown777/brick.git
cd brick
scons                        # build release
# ou
./build-release.sh           # release completo + extensão VS Code
```

O binário `brick` estará em `build/`.

### Executar um Demo

```bash
# Compilar e executar em um passo
brick run examples/hello.brc

# Ou compilar para binário primeiro
brick build examples/hello.brc -o hello
./hello
```

### Rodar Testes

```bash
scons test                   # todos os testes unitários
```

### Visualizar Memória

```bash
brick --visualize examples/hello.brc   # compila, executa, mostra TUI
brick --attach <pid>                  # anexa a processo rodando
```

---

## ⚡ Performance

### Bump Allocator

| Operação              | Tempo                 | vs malloc/free           |
|-----------------------|-----------------------|--------------------------|
| Alocação              | ~3 ciclos de CPU      | ~50–200× mais rápido     |
| Reset de bloco (64MB) | ~5 ns                 | 2000× mais rápido        |

**Benchmark real:**

```
Block alloc: 1.000.000 allocs de 64B em 0.002s   ← 19,5× mais rápido
malloc:      1.000.000 allocs de 64B em 0.039s   ← baseline
```

### Compilador

| Entrada        | Tempo de Compilação |
|----------------|---------------------|
| 100 structs    | 5 ms                |
| 1.000 linhas   | ~10 ms              |

---

## 📝 Exemplos

### Tipos de Largura Fixa e Interface

```brick
package EXEMPLO

using IO

block global = 64MB

interface Desenhavel {
    fn desenhar()
}

struct Circulo extends Desenhavel {
    u32 id
    f32 raio

    fn Circulo(u32 i, f32 r) {
        id = i
        raio = r
    }

    fn desenhar() {
        print("Círculo #{0} raio={1}", id, raio)
    }
}

fn main() {
    Circulo c = Circulo(1u32, 5.0f32)
    c.desenhar()
}
```

### Hot Reload

```bash
# Compilar com suporte a hot reload
brick build jogo.brc --release -o jogo

# Executar — Brick monitora arquivos via inotify
# Edite seu .brc e salve — o binário recarrega automaticamente
./jogo
```

### Macros (Geração de Código)

Gere código em tempo de compilação com `macro`, `build` e `emit`:

```brick
macro troca(a, b) {
    __tmp = $a
    $a = $b
    $b = __tmp
}

macro enum(nome, valores...) {
    emit { $nome = 0 }
    for i = 1; i < 4; i = i + 1 {
        emit { ${"$" ++ valores[i]} = i }
    }
}

enum(TipoArma, ARMA_SOCO, ARMA_PISTOLA, ARMA_RIFLE, ARMA_FOGUETE)

fn main() {
    x = 10; y = 20
    troca(x, y)        // x=20, y=10
    
    if arma == ARMA_FOGUETE {
        print("Fogo!")
    }
}
```

- **`macro`** — templates de código com interpolação `$`
- **`build { }`** — computação em tempo de compilação (matemática, loops, reflection)
- **`emit { }`** — gera saída de dentro de macros ou build
- **`$nome`** — insere valor do argumento
- **Varargs** — `valores...` captura argumentos restantes como lista
- **Higiene** — variáveis `__` ganham nomes únicos automaticamente

Veja o [Guia de Macros](docs/MACROS.pt-BR.md) para exemplos completos.

---

## 📁 Estrutura do Projeto

| Diretório      | Conteúdo                                               |
|----------------|--------------------------------------------------------|
| `src/`         | Compilador em C++20 (Lexer, Parser, Codegen)            |
| `runtime/`     | Runtime C (alocador de blocos, IO, hot reload)          |
| `visualizer/`  | TUI ncurses para visualização de memória                |
| `debugger/`    | Pretty-printers GDB, comandos custom, `.gdbinit`        |
| `examples/`    | Programas `.brc` de exemplo                              |
| `tests/`       | Testes unitários (SCons)                                |
| `benchmarks/`  | Scripts de benchmark                                    |
| `vscode-ext/`  | Extensão VS Code (highlight, LSP, memory view)          |
| `docs/`        | Site GitHub Pages (HTML + assets)                       |
| `wiki/`        | Fonte do GitHub Wiki                                    |
| `tasks/`       | 11 tarefas de desenvolvimento com estado por tarefa     |
| `build/`       | Artefatos de compilação                                 |

---

## 📚 Documentação

- **[Primeiros Passos](docs/GETTING_STARTED.pt-BR.md)** — Instalação, primeiro programa, uso da CLI
- **[Referência da Linguagem](docs/LANGUAGE.pt-BR.md)** — Sintaxe completa, tipos, pacotes, modelo de memória
- **[Arquitetura](docs/ARCHITECTURE.pt-BR.md)** — Como compilador, runtime e ferramentas se encaixam
- **[Guia de Hot Reload](docs/hot-reload.pt-BR.md)** — Troca de código ao vivo via dlopen + inotify
- **[Guia de Macros](docs/MACROS.pt-BR.md)** — Geração de código em tempo de compilação com `macro`/`build`/`emit`
- **[Otimizações](docs/OPTIMIZATIONS.pt-BR.md)** — Ajustes de performance e benchmarks
- 🇬🇧 **[English](README.md)** — English documentation

---

## 🤝 Contribuindo

O projeto é dividido em **11 tarefas**, cada uma com seu `AGENTS.md` e `STATE.md`.

Participe — abra uma issue ou PR em [github.com/nonunknown777/brick](https://github.com/nonunknown777/brick).

---

## 🧱 O Nome

**BriCk** é um jogo de palavras em três camadas:

1. **Brick** (tijolo) — blocos de memória são os tijolos que constroem o runtime
2. **C** no meio — compila para **C**
3. **BR** na frente — **Brasil**, a origem do projeto

> *BriCk — the brazilian C.*

---

## 📄 Licença

MIT License.

---

<p align="center">
  <sub>Feito com ❤️ em C++20 e C — porque às vezes você precisa ser rápido.</sub>
</p>

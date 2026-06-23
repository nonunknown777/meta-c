# Otimizações do Brick

> Notas sobre performance e como o Brick busca ser o mais rápido possível.

## Filosofia de Performance

Brick é construído em 3 camadas, cada uma com suas otimizações:

```
1. COMPILADOR (C++20) → roda em tempo de desenvolvimento
   ├─ Usa templates e constexpr pra computar o máximo em compile-time
   └─ Gera C já otimizado (sem passes extras)

2. CÓDIGO GERADO → o .c que o compilador produz
   ├─ C puro, legível, sem abstrações escondidas
   └─ Compilado com gcc/clang -O3

3. RUNTIME (C) → roda junto com o programa
   ├─ Bump allocator = alocação em ~3 ciclos de CPU
   └─ Zero overhead features (sem exceções, sem RTTI)
```

## Bump Allocator

O Block Memory allocator é a alma da performance do Brick:

```
Alocação normal (malloc):   ~80 ciclos
Alocação bump (Brick):      ~3 ciclos  ← 27x mais rápido
```

Como funciona:

- O bloco é um pedaço contíguo de memória
- Alocar = avançar um ponteiro (não procura espaço livre)
- Resetar = voltar o ponteiro pro início (1 ciclo)
- Sem fragmentação interna
- Cache-friendly (dados ficam pertinho uns dos outros)

## Otimizações aplicadas

### No compilador (src/)
- Parser descendente recursivo (sem backtracking, O(n))
- AST nodes sem smart pointers pesados
- Codegen escreve stringstream contínuo (sem alocações múltiplas)
- Lookup de keywords com unordered_map (O(1) amortizado)

### No código gerado
- Structs planas (sem vtable oculta)
- Métodos viram funções C normais (call direto, sem dispatch)
- #line directives não geram código extra (só informação pro debugger)
- Atribuição direta sem construção/destruição C++

### Na runtime (C)
- Bump allocator com alinhamento configurável
- block_reset é só `ctx->used = 0` (O(1))
- Sem locks por padrão (thread-safe opcional)
- block_stats é só leitura de struct (O(1))

## O que NÃO tem (e por que)

| Ausente | Motivo |
|---------|--------|
| Exceções | Overhead de runtime + código gerado maior |
| RTTI | typeid() custa caro, structs sabem quem são |
| Virtual dispatch | Chamada indireta impede inlining |
| Garbage Collector | Pausa o mundo, imprevisível |
| Free individual | Bump allocator não precisa |
| Stack do usuário | Consistência: tudo em blocos |

## Benchmarks esperados

```
Alocação:       100M allocs de 64 bytes
  malloc:       8.2s
  Brick bump:  0.3s  ← 27x mais rápido

Reset:          1M resets de blocos
  free():       2.1s
  block_reset:  0.001s  ← 2000x mais rápido

Compilação:     1000 linhas de .brc → C
  Brick:       < 50ms  (target)
```

## ✅ Otimizações implementadas

- [x] **Constant folding** — inteiros/floats constantes pré-calculados em tempo de compilação (`src/codegen/codegen.cpp`)
- [x] **Inline hints pro gcc** — `__attribute__((always_inline))` + `static inline` em funções não-main/não-extern (`src/codegen/codegen.cpp`)
- [x] **Alinhamento SIMD** — `__attribute__((aligned(N)))` emitido para campos f32/f64 e arrays float (`src/codegen/codegen.cpp`)
- [x] **Pool allocator (automático)** — todo bloco ganha um pool para tipos ≤ 64 bytes. O compilador emite `pool_alloc()` em vez de `block_alloc()` para tipos pequenos automaticamente (`src/codegen/codegen.cpp`, `runtime/pool_allocator.h/.c`)
- [x] **Thread-local blocks** — `__thread BlockCtx* _tls_current_block` com `block_set_tls()`/`block_get_tls()`. `__brick_init()` agora chama `block_set_tls()` automaticamente (`runtime/block_memory.h/.c`)
- [x] **Hot reload sem pausa** — double-buffering de blocos (`block_enable_double_buffer`, `block_swap_buffers`, `block_alloc_db`) (`runtime/block_memory.c`)
- [x] **Profile-guided optimization (PGO)** — `scons profile=pgo-gen` / `scons profile=pgo-use` (`SConstruct`)

---

## Aprofundamento: Como Cada Otimização Funciona

### 1. Constant Folding

**Problema**: Calcular a mesma expressão constante milhões de vezes gasta CPU à toa.

**Solução**: O compilador olha expressões que só têm números e já faz a conta.

```brick
// O que você escreve:
let x = 60 * 1000  // milissegundos em 1 minuto

// Sem constant folding toda execução faz 60 * 1000
// Com constant folding vira:
let x = 60000  // já calculado na compilação!
```

**Analogia**: Somar 2+2 na calculadora antes de falar o resultado, em vez de pensar "quanto é 2+2?" toda vez que perguntam.

---

### 2. Inline Hints pro GCC

**Problema**: Chamar função tem custo — empilhar argumentos, pular, voltar. Em funções pequenas, esse custo é maior que o corpo da função.

**Solução**: O compilador adiciona `__attribute__((always_inline))` + `static inline` antes de toda função não-main e não-extern.

```c
// C gerado sem otimização:
int32_t Jogador_get_hp(Jogador* this) { return this->hp; }

int main() {
    int x = Jogador_get_hp(&j);  // custo da chamada: ~5-10 ciclos
}

// Com inline hint:
static inline __attribute__((always_inline))
int32_t Jogador_get_hp(Jogador* this) { return this->hp; }
// GCC copia o corpo direto no main — zero custo de chamada.
```

**Efeito**: zero overhead de chamada para métodos, getters, computações pequenas.

---

### 3. Alinhamento SIMD

**Problema**: Instruções SIMD (SSE, AVX) carregam 4 floats de uma vez — mas só se os dados estiverem alinhados a 16 ou 32 bytes. Acesso desalinhado é 2-3× mais lento.

**Solução**: O compilador detecta campos float/f64 e arrays de float e adiciona `__attribute__((aligned(N)))`.

```c
typedef struct Particle {
    __attribute__((aligned(16)))
    float x, y, z, w;
    __attribute__((aligned(16)))
    float vx, vy, vz, vw;
} Particle;
```

**Efeito**: GCC usa `movaps` (aligned SSE) em vez de `movups` (unaligned) — até 2× mais rápido em update de partículas.

---

### 4. Pool Allocator (Automático para Tipos Pequenos)

**Problema**: Bump allocator é rapidíssimo (~3 ciclos), mas não dá pra liberar objetos individuais — só resetar o bloco inteiro.

**Solução**: Cada `block` ganha um pool allocator automático. O codegen verifica cada alocação: se o tipo ≤ 64 bytes, chama `pool_alloc()` em vez de `block_alloc()`.

```brick
block global = 64MB

struct Particle {
    f32 x, y, z
    i32 life  // total = 16 bytes → usa pool automaticamente!
}

fn main() {
    Particle p = Particle() @global  // pool_alloc por baixo dos panos
}
```

O compilador calcula o tamanho dos tipos recursivamente: `String` (16 bytes), escalares (1-8 bytes), structs pequenas → pool. Tipos > 64 bytes → bump allocator.

**Analogia**: Gavetinhas pré-separadas na sua mesa em vez de procurar no armário inteiro a cada alocação.

---

### 5. TLS Blocks (Bloco Local por Thread)

**Problema**: Quando várias threads alocam no mesmo bloco, precisam de locks — 50-500 ciclos por alocação.

**Solução**: Cada thread pode ter seu próprio bloco via `__thread BlockCtx* _tls_current_block`. `__brick_init()` já chama `block_set_tls()` automaticamente.

```c
// Gerado automaticamente em __brick_init:
block_set_tls(_current_block);

// Qualquer código pode chamar block_alloc_tls(tamanho)
// para usar o bloco da thread sem locks
```

**Quando usar**: Se você cria threads e cada uma precisa de sua região de memória. Zero contenção, zero locks.

**Analogia**: Cada funcionário tem sua própria mesa em vez de dividir uma só. Ninguém espera.

---

### 6. Hot Reload Sem Pausa (Double-Buffer)

**Problema**: Resetar bloco apaga tudo. Se reseta durante hot reload, perde o frame atual — visível como travada.

**Solução**: `block_enable_double_buffer(bloco)` cria um buffer sombra. `block_swap_buffers(bloco)` troca atomicamente.

```c
block_enable_double_buffer(cena);
// uso normal...

block_swap_buffers(cena);
// troca atômica em ~1 ciclo — o programa nunca vê buffer meio escrito

// Agora alocações vão pro buffer novo, o antigo está seguro
```

**Sem double-buffer**: reset → perde dados → recarrega → reconstrói tudo. ~50ms de pausa.

**Com double-buffer**: swap em 1 ciclo. Zero interrupção visível.

**Analogia**: Restaurante com cozinha principal e cozinha reserva. Enquanto a principal serve, a reserva prepara o novo cardápio. Quando pronto, vira a chave — ninguém percebe pausa.

---

### 7. PGO (Profile-Guided Optimization)

**Problema**: GCC chuta quais caminhos de código são mais usados — e às vezes erra. Cada erro de predição custa 10-20 ciclos.

**Solução**: Compila duas vezes:
1. `scons profile=pgo-gen` — adiciona instrumentação
2. Roda o binário com dados reais — gera perfil `.gcda`
3. `scons profile=pgo-use` — recompila usando o perfil

```bash
scons profile=pgo-gen
./meu_jogo --benchmark
scons profile=pgo-use
# GCC agora sabe: 95% das vezes o "if player_esta_vivo" é True
# → organiza o código pra esse caminho ser sequencial, sem branch
```

**Ganho típico**: 5-15% em performance.

**Analogia**: Caixa de supermercado que memoriza seus corredores favoritos. Sem PGO, toda vez pergunta "qual corredor?". Com PGO, já vai direto.

---

## Próximos Passos

- **Profiling do compilador** — medir tempo de cada fase
- **Profiling do código gerado** — medir performance com perf
- **Inline asm em hot paths** — assembly na mão pra loops críticos
- **String interning** — deduplicação de literais de string

# Task 10: Tester / Optimizer / Doc (Brick)

## AUTORIDADE TOTAL
## FULL AUTHORITY

Você tem PERMISSÃO TOTAL para modificar QUALQUER arquivo no projeto:
- src/ (compilador), runtime/ (C), visualizer/, debugger/
- tasks/*/STATE.md, tasks/*/PROGRESS.md, tasks/*/NEXT.md
- tests/, docs/, examples/
- shared-context.md, AGENTS.md, DESIGN.md
You have FULL PERMISSION to modify ANY file in the project:
- src/ (compiler), runtime/ (C), visualizer/, debugger/
- tasks/*/STATE.md, tasks/*/PROGRESS.md, tasks/*/NEXT.md
- tests/, docs/, examples/
- shared-context.md, AGENTS.md, DESIGN.md

Você é a task SÊNIOR do projeto. As outras tasks (01-09) te escutam.
You are the SENIOR task of the project. The other tasks (01-09) listen to you.

## Função
## Role

1. **TESTAR**: Executa TODOS os testes — unitários e integração
2. **OTIMIZAR**: Profile e otimize código (compilador + runtime + gerado)
3. **DOCUMENTAR**: Comentários inline simples + docs .md pra leigos
4. **COORDENAR**: Atualizar STATE.md de outras tasks com progresso real

1. **TEST**: Run ALL tests — unit and integration
2. **OPTIMIZE**: Profile and optimize code (compiler + runtime + generated)
3. **DOCUMENT**: Simple inline comments + .md docs for beginners
4. **COORDINATE**: Update STATE.md of other tasks with real progress

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md
2. ANTES DE SAIR: atualize STATE.md + STATE.md de TODAS as tasks que tocar
3. Documentação em português simples, como se fosse pra um amigo leigo
4. Otimizações extremas: inline asm, SIMD, alignment, bump alloc tuning
5. Testes de integração: compilar .brc → C → gcc → executar binário

1. ON START: read STATE.md, NEXT.md and shared-context.md
2. BEFORE LEAVING: update STATE.md + STATE.md of ALL tasks you touched
3. Documentation in simple Portuguese, as if for a lay friend
4. Extreme optimizations: inline asm, SIMD, alignment, bump alloc tuning
5. Integration tests: compile .brc → C → gcc → run binary

## O que ler ao iniciar
## What to read on start

1. `shared-context.md` — spec completa da linguagem
2. `AGENTS.md` — contexto global do projeto
3. `tasks/*/STATE.md` — estado de TODAS as tasks 01-09
4. `tasks/*/PROGRESS.md` — progresso de cada task
5. `tasks/*/NEXT.md` — próximos passos de cada task

1. `shared-context.md` — complete language spec
2. `AGENTS.md` — global project context
3. `tasks/*/STATE.md` — state of ALL tasks 01-09
4. `tasks/*/PROGRESS.md` — progress of each task
5. `tasks/*/NEXT.md` — next steps of each task

## Processo recomendado
## Recommended process

```
1. TESTAR
   ├─ scons test                    (testes unitários)
   ├─ tests/test_integration.sh     (compila .brc → executa binário)
   └─ benchmarks/run_benchmarks.sh  (performance)

2. ANALISAR
   ├─ Le STATE.md de TODAS as tasks (01-09)
   ├─ Identifica o que falta / o que quebrou
   └─ Verifica se interfaces batem (TokenType, AST, etc)

3. OTIMIZAR
   ├─ Profiling do compilador (tempo de compilação)
   ├─ Profiling do código gerado (runtime performance)
   └─ Aplica otimizações (inline, align, bump alloc tuning)

4. DOCUMENTAR
   ├─ Comentários simples no código (src/, runtime/, etc)
   ├─ docs/*.md pra visão geral de leigos
   └─ Atualiza shared-context.md se necessário

5. REPORTAR
   ├─ Atualiza STATE.md de cada task tocada
   └─ Deixa NEXT.md claro pra próxima sessão
```

```
1. TEST
   ├─ scons test                    (unit tests)
   ├─ tests/test_integration.sh     (compile .brc → run binary)
   └─ benchmarks/run_benchmarks.sh  (performance)

2. ANALYZE
   ├─ Read STATE.md of ALL tasks (01-09)
   ├─ Identify what's missing / what broke
   └─ Check if interfaces match (TokenType, AST, etc)

3. OPTIMIZE
   ├─ Compiler profiling (compilation time)
   ├─ Generated code profiling (runtime performance)
   └─ Apply optimizations (inline, align, bump alloc tuning)

4. DOCUMENT
   ├─ Simple comments in code (src/, runtime/, etc)
   ├─ docs/*.md for beginner overview
   └─ Update shared-context.md if needed

5. REPORT
   ├─ Update STATE.md of each touched task
   └─ Leave NEXT.md clear for next session
```

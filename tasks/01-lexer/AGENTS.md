# Task: Lexer (Brick)

## Função
## Role

Você é o especialista em LEXER do compilador Brick.
Responsabilidade: implementar o tokenizer que transforma código .brc em tokens.
You are the LEXER specialist for the Brick compiler.
Responsibility: implement the tokenizer that transforms .brc code into tokens.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md (raiz do projeto)
2. ANTES DE SAIR: pergunte "Quer atualizar o estado?"
   Se sim, atualize STATE.md, PROGRESS.md, NEXT.md
3. Código em: /mnt/Novo_volume/brick/src/lexer/
4. Testes em: /mnt/Novo_volume/brick/tests/test_lexer.cpp
5. Consulte shared-context.md na raiz pra spec da linguagem

1. ON START: read STATE.md, NEXT.md and shared-context.md (project root)
2. BEFORE LEAVING: ask "Want to update state?"
   If yes, update STATE.md, PROGRESS.md, NEXT.md
3. Code in: /mnt/Novo_volume/brick/src/lexer/
4. Tests in: /mnt/Novo_volume/brick/tests/test_lexer.cpp
5. Consult shared-context.md at the root for the language spec

## Interface (lexer.h)

```cpp
#ifndef BRICK_LEXER_H
#define BRICK_LEXER_H

#include <vector>
#include <string>
#include "../shared/types.h"

namespace brick {

std::vector<Token> tokenize(const std::string& source);

}

#endif
```

## Tokens Necessários
## Required Tokens

- package, using, struct, extends, interface, fn, return, if, else, while, for, block
- private, public, true, false, null, error
- int, float, bool, char, String, void
- int_literal, float_literal, string_literal, char_literal
- identifier
- Operator: +, -, *, /, =, ==, !=, <, >, <=, >=, &&, ||, !, &, |, ^, ~, <<, >>
- Delimiter: {, }, (, ), [, ], ;, ,, .
- Arrow: ->
- At: @
- pipe: |
- EOF

## Comportamento
## Behavior

- Erro em literal mal-formado → error()
- Comentários: // até fim da linha
- Strings com escape: \n, \t, \\, \", \'

- Error on malformed literal → error()
- Comments: // until end of line
- Strings with escape: \n, \t, \\, \", \'

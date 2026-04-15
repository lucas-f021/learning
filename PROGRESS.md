# Interpreter Progress

## Project: Tree-walking interpreter in C (single file: learning.c)

## Pipeline
```
source text  →  [Lexer]  →  tokens  →  [Parser]  →  AST  →  [Evaluator]  →  value
```

## Tier 1 scope (calculator)
- Integers only (multi-digit, no floats)
- Operators: + - * /
- Parentheses supported
- Whitespace ignored
- REPL (read-eval-print loop)

---

## What's done in learning.c

### Includes
- stdio.h, stdlib.h, string.h, ctype.h

### TokenType enum (typedef'd)
TOK_INT, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_LPAREN, TOK_RPAREN, TOK_EOF

### Token struct (typedef'd)
- `TokenType type`
- `union { int int_val; } value` — tagged union, room to add more types later

### Lexer struct (typedef'd)
- `char *pos` — pointer to current position in source string

### next_token(Lexer *l) — partially done
- Skips whitespace with while loop
- Handles TOK_INT: saves start pointer, advances while isdigit, calls atoi, returns Token
- MISSING: cases for +, -, *, /, (, ), EOF, and null/unknown character handling

---

## What's next (pick up here)

1. Add the remaining switch cases to next_token:
   - `'+'` → TOK_PLUS (advance pos, return token)
   - `'-'` → TOK_MINUS
   - `'*'` → TOK_STAR
   - `'/'` → TOK_SLASH
   - `'('` → TOK_LPAREN
   - `')'` → TOK_RPAREN
   - `'\0'` → TOK_EOF (do NOT advance past null terminator)
   - default → error/unknown token

2. After lexer is complete: test it by printing token stream before building parser.

3. Then: parser (recursive descent, builds AST).

4. Then: evaluator (recursive tree walk).

---

## Key concepts covered so far
- Tagged union pattern for token payloads
- Lexer as a stateful pointer walker
- Parser drives, lexer serves (parser calls next_token)
- Tree shape encodes precedence — parentheses disappear after parsing
- Lower nodes in AST = evaluated first
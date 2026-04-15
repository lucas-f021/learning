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

### next_token(Lexer *l) — complete
- Skips whitespace
- Switch on `*l->pos`:
  - `'+' '-' '*' '/' '(' ')'` → delegated to `casehelper(l, TokenType)` which builds the Token, advances pos, returns by value
  - `'\0'` → TOK_EOF (does NOT advance — safe to call repeatedly after end of input)
  - `default` → if isdigit, read multi-digit int via atoi on a saved start pointer; else fprintf error + exit(1)

### casehelper(Lexer *l, TokenType type) — helper
- Collapses the repeated "build Token, advance pos, return" pattern for all single-char operator/paren tokens

### Test driver (main + print_token) — complete
- Hardcoded source string, Lexer pointed at it
- do-while loop: `t = next_token(&l); print_token(t);` until `t.type == TOK_EOF`
- `print_token` switches on type, prints human-readable name; TOK_INT also prints `value.int_val`
- Verified output on `"1 + 2 * (3-4)"` — all token types exercised, EOF terminates cleanly

### File reorganization — complete
- Code is now grouped by pipeline phase with banner comments: `/* ===== LEXER ===== */`, `/* ===== PARSER ===== */`, `/* ===== EVALUATOR ===== */`, `/* ===== MAIN ===== */`
- Normalized typedef brace style to `} Name;` with a space everywhere
- Easy future cut-points if we ever split into lexer.c/parser.c/eval.c

### Parser — partially started

**AST types (complete):**
- `OpType` enum: `OP_ADD, OP_SUB, OP_MUL, OP_DIV` (kept separate from TokenType to decouple lex/parse layers)
- `NodeType` enum: `NODE_INT, NODE_BINOP`
- `Node` struct (tagged union):
  - `NodeType type` tag
  - `union uni` with `int int_value` arm and `struct { OpType op; struct Node *left; struct Node *right; } binop` arm
  - Uses `typedef struct Node { ... } Node;` form so `struct Node *` self-references inside the body resolve correctly (the typedef alias `Node` doesn't exist until the closing line)

**Parser struct (complete):**
- `Lexer *l` — token source
- `Token curr` — one-token lookahead cache (the key new idea: parser needs to peek without consuming)

**`init_parser(Parser *p, Lexer *lex)` — complete:**
- Stores `lex` into `p->l`
- Primes `p->curr` by calling `next_token(lex)` once

**MISSING:**
- `peek(Parser *p)` → returns `p->curr` (one-liner)
- `advance(Parser *p)` → save current, refill via `next_token(p->l)`, return saved old token
- `parse_factor`, `parse_term`, `parse_expr` (recursive descent)
- AST printer for debugging (analogous to `print_token`)

---

## What's next (pick up here)

1. **Finish parser plumbing:** write `peek` and `advance` helpers, then the three recursive-descent functions.
   - Grammar for Tier 1:
     - `expr   → term   (('+' | '-') term)*`
     - `term   → factor (('*' | '/') factor)*`
     - `factor → INT | '(' expr ')'`
   - Bottom-up: `parse_factor` first (base case — INT or parenthesized expr), then `parse_term`, then `parse_expr`.
   - Tree shape encodes precedence — lower in call stack = lower in tree = evaluated first.
   - Left-associativity comes from the `(...)*` repetition: parse left, then while operator-of-this-level is next, consume it + parse right + wrap as new left.
   - Use `malloc` for nodes for now (arena allocator from prior project would fit perfectly here later).

2. **AST printer** — recursive walk that prints the tree shape (parenthesized form or indented). Verify the parser before writing the evaluator.

3. **Evaluator** — recursive tree walk, returns int. Switch on `node->type`; for BINOP, recursively eval children, then apply `op`.

4. **REPL** — read line, feed to lexer, parse, eval, print. Loop.

---

## Key concepts covered so far
- Tagged union pattern for token payloads
- Lexer as a stateful pointer walker
- Parser drives, lexer serves (parser calls next_token)
- Tree shape encodes precedence — parentheses disappear after parsing
- Lower nodes in AST = evaluated first
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

### Parser — complete

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

**`peek(Parser *p)` / `advance(Parser *p)` — complete:**
- `peek` returns `p->curr` without consuming
- `advance` saves current, refills `p->curr` via `next_token(p->l)`, returns the saved old token

**Recursive-descent functions — complete:**
- Forward declarations used so the three functions can call each other
- `parse_expr`: parses a `term`, then while current is `+`/`-` builds left-assoc BINOP (OP_ADD/OP_SUB), folding prev result as `left`
- `parse_term`: same shape for `*`/`/` (OP_MUL/OP_DIV) over `factor`s
- `parse_factor`: if INT → leaf node; if `(` → advance, recurse into `parse_expr`, advance past `)`; else error + exit
- Nodes allocated with `malloc`

### AST printer — complete
- `print_ast(Node *n)` — recursive walk; NODE_INT prints the integer, NODE_BINOP prints `left op right` inline (infix)

### Evaluator — complete
- `eval(Node *n)` → int
- NODE_INT returns the stored int
- NODE_BINOP recursively evals both children, then switches on `op` to combine

### Arena allocator integration — complete
- `arena.h` / `arena.c` — interface/implementation split with header guard (`#ifndef ARENA_H`), pulled from prior arenaalloc.c project
- Public API: `arena_create(size)`, `arena_destroy(a)`, `arena_reset(a)`, `arena_alloc(a, size)` (8-byte-aligned bump alloc, NULL on overflow)
- `Parser` struct gained an `Arena *arena` field; `init_parser` takes and stores it
- All three `malloc(sizeof(Node))` calls in `parse_expr` / `parse_term` / `parse_factor` swapped to `arena_alloc(p->arena, sizeof(Node))`
- `main` creates a 4KB arena up front, calls `arena_reset(a)` after each REPL line to invalidate that line's AST, `arena_destroy(a)` on Ctrl-D
- Zero leaks, no recursive `free_ast` walker needed — all nodes from one line share a lifetime, bulk-freed at once
- Compile: `gcc -Wall -Wextra learning.c arena.c -o learning`

### REPL — complete
- `main` loops reading lines with `fgets(x, 128, stdin)`, breaks on EOF (Ctrl-D)
- Parser and Lexer declared once outside the loop, re-initialized each iteration via `init_parser`
- Prints `eval(parse_expr(&p))` for each line, then resets the arena

**Verified end-to-end on:**
- `1+2` → 3
- `3*4+5` → 17 (precedence: `*` tighter than `+`)
- `(1+2)*3` → 9 (parens override precedence)
- `10/2-1` → 4 (left-associativity + mixed precedence)

**Tier 1 is feature-complete.** Integer calculator with `+ - * /`, parens, whitespace, REPL. Tree-walking pipeline (lex → parse → eval) works end-to-end.

**Compile warnings seen:** `print_token` and `peek` are unused — both were scaffolding (`print_token` from lexer testing; `peek` never ended up called because parse functions inspect `p->curr` directly). Keep for debugging or delete.

---

## What's next (pick up here)

Tier 1 calculator is functionally complete. Possible next directions:

1. **Review pass** — loose ends worth looking at:
   - `parse_factor`'s `(` branch advances past the closing token without checking it's actually `)` — malformed input like `(1+2` won't be caught here
   - `eval`'s NODE_BINOP switch has no return after the switch; all enum cases are covered but the compiler may still warn
   - ~~Each REPL iteration leaks the AST~~ — fixed by arena integration
   - Division by zero not handled
   - `arena_alloc` return values aren't NULL-checked — if a single expression exceeds 4KB of nodes, you'll deref NULL. Unlikely in practice but a `checked_alloc` wrapper would harden it.

2. **Tier 2 ideas** (pick when ready): unary minus, variables + assignment, floats, comparison operators, booleans, `if`/`else`, blocks, functions.

---

## Tier 2 — Variables (in progress)

### Scope decision
- Multiple statements per line, `;` mandatory after every statement
- New syntax: `let NAME = EXPR ;` for binding, bare `EXPR ;` for eval-and-print
- Grammar additions:
  - `program   → statement*`
  - `statement → 'let' IDENT '=' expr ';' | expr ';'`
  - `factor    → INT | IDENT | '(' expr ')'` (added IDENT)

### Ownership decision — kept arena and hashmap separate
- Hashmap stays malloc-based (uses `strdup` for keys, `free` in destroy/delete/resize)
- Arena stays strictly for AST nodes, reset per line
- Environment (hashmap) will live for the whole session, independent memory
- Reason: hashmap has individual per-key mutation (delete/overwrite) which fits malloc/free better than arena; keeping them decoupled avoids the "resize leaks the old bucket array into the arena" problem and keeps `hash.c` project-agnostic

### Hashmap extraction — complete
- Pulled the prior `customhash.c` project into `hash.h` / `hash.c` (same split pattern as arena)
- Header guard (`#ifndef HASH_H`), prototypes for `hm_create`, `hm_destroy`, `hm_insert`, `hm_get`, `hm_delete` only (internal helpers like `hm_resize` / `hashfnv1a` stay `static` in `hash.c`, NOT declared in header — `static` in a shared header is a category error)
- Header includes `<stdbool.h>` + `<stdint.h>` so it stands alone
- `customhash.c` deleted to avoid double-main collision
- `learning.c` now `#include "hash.h"`
- Compile: `gcc -Wall -Wextra learning.c arena.c hash.c -o learning`

### Lexer additions — complete
- Four new `TokenType`s: `TOK_IDENT`, `TOK_LET`, `TOK_EQ`, `TOK_SEMI`
- `Token` union gains a second arm: `char *ident` (null-terminated identifier name)
- Switch in `next_token`:
  - `'='` → `casehelper(l, TOK_EQ)`
  - `';'` → `casehelper(l, TOK_SEMI)`
  - Default branch grew an `else if (isalpha(*l->pos))` path: scan `isalnum` run, compute length via pointer subtraction, check `len == 3 && strncmp(tmp, "let", 3) == 0` for the `let` keyword, else produce `TOK_IDENT` with `strndup(tmp, len)` for the name
- `print_token` extended with the four new cases (all end with `\n`, `break;` after every case)

**Verified on** `"let x = 5; x + 1"` (temporary lex-only main that did `next_token` + `print_token` in a loop):
```
LET
IDENT(x)
EQ
INT(5)
SEMI
IDENT(x)
PLUS
INT(1)
EOF
```

Reverted `main` back to the real parse+eval REPL before committing.

### Known lexer issue to come back to later
- `TOK_IDENT` names are `strndup`'d, so they escape the arena lifecycle — every line leaks the names because nothing `free`s them. Will tighten once the full pipeline works.

---

## What's next (Tier 2 pickup)

1. **AST additions:** add `NODE_IDENT` and `NODE_LET` to `NodeType`; extend the `Node` union with two new arms (ident carries a `char *name`; let carries `char *name` + `Node *value`).

2. **Parser additions:**
   - `parse_factor` grows a `TOK_IDENT` case (structurally identical to the `TOK_INT` case, just stores a string). Simplest approach: point NODE_IDENT's name directly at the strndup'd memory from the Token. Leaks per line — fine for now.
   - New function `parse_let`: advance past `let`, expect IDENT (save name), expect EQ, parse_expr for RHS, expect SEMI, build NODE_LET.
   - New function `parse_statement`: if curr is TOK_LET → parse_let; else → parse_expr then consume SEMI.
   - Forward-declaration block at top of parser section needs to grow.

3. **REPL loop change:** instead of `parse_expr` once per line, loop `parse_statement` until `p->curr.type == TOK_EOF`. Each statement gets `eval`'d.

4. **Environment wiring:** `HashMap *env` created once in `main` (before the REPL loop), destroyed on Ctrl-D. Threaded as a parameter into `eval`. NEW: `eval` signature becomes `int eval(Node *n, HashMap *env)`.

5. **Evaluator additions:**
   - `NODE_IDENT`: `hm_get(env, name, &out)`; if not found → error ("undefined variable"); else return `out`.
   - `NODE_LET`: recursively eval the value subtree, then `hm_insert(env, name, value)`. Decision: what should a let statement "return"? Probably 0, or just don't print let-statement results in the REPL (track which kind of node was evaluated and skip the `printf("%d\n", ...)` for lets).

---

## Key concepts covered so far
- Tagged union pattern for token payloads
- Lexer as a stateful pointer walker
- Parser drives, lexer serves (parser calls next_token)
- Tree shape encodes precedence — parentheses disappear after parsing
- Lower nodes in AST = evaluated first
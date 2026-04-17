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

### REPL — complete
- `main` loops reading lines with `fgets(x, 128, stdin)`, breaks on EOF (Ctrl-D)
- Builds a fresh `Lexer` and `Parser` per iteration, pointing lexer at the input buffer
- Prints `eval(parse_expr(&p))` for each line

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
   - Each REPL iteration leaks the AST (no recursive free) — fine for tier 1, but an arena would solve it cleanly
   - Division by zero not handled

2. **Tier 2 ideas** (pick when ready): unary minus, variables + assignment, floats, comparison operators, booleans, `if`/`else`, blocks, functions.

---

## Key concepts covered so far
- Tagged union pattern for token payloads
- Lexer as a stateful pointer walker
- Parser drives, lexer serves (parser calls next_token)
- Tree shape encodes precedence — parentheses disappear after parsing
- Lower nodes in AST = evaluated first
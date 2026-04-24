# Interpreter Progress

## Project: Tree-walking interpreter in C (single file: learning.c)

## Pipeline
```
source text  →  [Lexer]  →  tokens  →  [Parser]  →  AST  →  [Evaluator]  →  value
```

## Project roadmap
- **Tier 1 — Calculator** (complete): integers, `+ - * /`, parens, whitespace, REPL
- **Tier 2 — Variables** (complete): `let NAME = EXPR;`, identifiers, multi-statement lines, session environment
- **Tier 3 — Booleans + if/else** (next): comparison ops, control flow
- **Tier 4 — Functions + closures**: first-class functions, lexical scoping, captured environments
- **Final step — File execution**: run `.lang` files instead of (or alongside) REPL

---

## Current state snapshot (after Tier 2)

**Files:** `learning.c` (single-file interpreter), `arena.h`/`arena.c` (bump allocator for AST), `hash.h`/`hash.c` (string→int environment), `.gitignore`, `PROGRESS.md`.

**Compile:** `gcc -Wall -Wextra learning.c arena.c hash.c -o learning`

**`TokenType`:** `TOK_INT, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_LPAREN, TOK_RPAREN, TOK_EOF, TOK_IDENT, TOK_LET, TOK_EQ, TOK_SEMI`

**`Token`:** tagged union, `type` + `{ int int_val; char *ident; } value`. `ident` is `strndup`'d by the lexer (leaks per line — known issue).

**`Lexer`:** `char *pos` into source buffer. `next_token` skips whitespace, switches on current char, delegates single-char ops to `casehelper`, handles multi-digit ints via `isdigit` + `atoi`, handles identifiers via `isalpha`/`isalnum` + `strndup` + `let`-keyword check.

**`NodeType`:** `NODE_INT, NODE_BINOP, NODE_IDENT, NODE_LET`

**`Node`:** tagged union, `type` + `uni` with arms for int_value, binop (op + left/right `struct Node *`), ident_name, and let (name + value subtree).

**`Parser`:** `{ Lexer *l; Token curr; Arena *arena; }`. Primed via `init_parser(p, lex, arena)` which calls `next_token` once. `advance` saves curr, refills via `next_token`, returns old.

**Parse functions** (all arena-allocate their nodes, all NULL-check):
- `parse_statement` → dispatches to parse_let or parse_expr+SEMI
- `parse_let` → `let IDENT = EXPR ;` → NODE_LET
- `parse_expr` / `parse_term` / `parse_factor` → recursive-descent left-assoc over `+-` / `*/` / atoms
- `parse_factor` handles INT, IDENT (NODE_IDENT), `(expr)` with verified `)` close

**`eval(Node *n, HashMap *env)` → int:** switches on node type. NODE_INT returns payload, NODE_IDENT does `hm_get` (errors if not found), NODE_LET recursively evals value then `hm_insert`, NODE_BINOP recursively evals both sides + applies op.

**`main`:** creates 4KB arena + session HashMap once, REPL loop reads line, inits parser, loops `parse_statement` + `eval` over the line's statements, prints result only if node wasn't NODE_LET, resets arena per line. On Ctrl-D: destroys both arena and env.

**Verified on:**
- `1+2` → 3; `3*4+5` → 17; `(1+2)*3` → 9 (Tier 1)
- `let x = 5; x + 1;` → 6; `let y = x * 2; y;` → 10 (Tier 2 variables, cross-line persistence)

**Known debt:**
- `strndup`'d identifier names leak per line (lexer side)
- NODE_LET returns 0 as placeholder
- Division by zero / overflow not handled
- `print_token` unused (kept as debug scaffolding)

---

## History — how we got here (each section reflects state at time of build; see snapshot above for current truth)

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

**`advance(Parser *p)` — complete:**
- Saves current, refills `p->curr` via `next_token(p->l)`, returns the saved old token
- (A `peek` helper was written but never used — parse functions read `p->curr` directly, so it was deleted.)

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

## Tier 2 — Variables (complete)

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

### AST additions — complete
- `NodeType` now: `NODE_INT, NODE_BINOP, NODE_IDENT, NODE_LET`
- Two new arms in the `Node` union:
  - `struct { char *name; } ident` — name lookup reference (field accessed as `n->uni.ident_name`)
  - `struct { char *name; struct Node *value; } let` — name + RHS subtree pointer (note `struct Node *`, not `Node *`, to sidestep the self-reference gotcha)

### Parser additions — complete
- `parse_factor` grew a `TOK_IDENT` case — allocate NODE_IDENT, point name at the Token's `strndup`'d ident (shared — leaks per line, fine for now)
- `parse_let` — advance past `let`, expect+save IDENT name, expect+advance `=`, `parse_expr` for RHS, expect+advance `;`, build NODE_LET
- `parse_statement` — if curr is TOK_LET → delegate to parse_let; else parse_expr first, then check+advance the trailing SEMI (defensive: errors if SEMI missing)
- Forward-declaration block at top of PARSER section grew to cover the two new functions

### Evaluator additions — complete
- `eval` signature changed to `int eval(Node *n, HashMap *env)` — env threads through every recursive call
- NODE_IDENT: `hm_get(env, name, &out)` — capture bool in `found`, error if not found, else return `out`
- NODE_LET: recursively eval the value subtree into an int, `hm_insert(env, name, val)`, return 0 as placeholder
- Binop recursions updated to pass `env` down

### REPL loop rewrite — complete
- `HashMap *env = hm_create()` before the outer `while(1)` — session-long
- Inner `while(p.curr.type != TOK_EOF)` loop: `parse_statement` → `eval` (stored result) → print only if the stmt isn't NODE_LET
- `arena_reset(a)` once per line, after the statements loop exits (not per statement)
- `hm_destroy(env)` + `arena_destroy(a)` only on Ctrl-D exit — never per line, never per statement

**Verified end-to-end on:**
- `let x = 5; x + 1;` → 6 (binding + use on same line)
- `let y = x * 2; y;` → 10 (x persists across lines, new var bound, printed via bare-ident expr stmt)
- `5 + 3;` → 8 (plain expression statements still work)
- `let x = 5;` alone → no output (NODE_LET suppresses print)
- `x;` alone → prints stored value (bare ident as expression statement)

**Tier 2 is feature-complete.** Variables, let bindings, expression statements, session-persistent environment. All four node types (INT, BINOP, IDENT, LET) exercised through the full lex → parse → eval pipeline.

---

## What's next

### Loose ends (optional cleanup)
- Identifier name strings `strndup`'d in lexer leak every line (nothing frees them); not a runtime issue, but not clean. Could arena-copy in parse_factor and let the arena reset handle it — but then the hashmap's `strdup` would still copy it into malloc territory anyway, so the lexer-side leak persists until something frees it.
- NODE_LET returning 0 is a placeholder; could use a "void statement" marker or rely on type-tag inspection (current approach).
- Division by zero, overflow, and uninitialized-variable edge cases still unhandled.
- `print_token` still unused (kept as scaffolding).

### Tier 3 — Booleans + if/else (next)
- New lexer tokens: `TOK_TRUE`, `TOK_FALSE`, `TOK_IF`, `TOK_ELSE`, `TOK_LT`/`TOK_GT`/`TOK_EQEQ`, `TOK_LBRACE`/`TOK_RBRACE`
- AST: `NODE_BOOL`, `NODE_IF` (condition + then-branch + else-branch children), comparison ops added to BINOP
- Evaluator: booleans as int (0/1) is fine for tier 3; `if` dispatches based on condition
- Teaching payoff: control flow, branching, value vs statement distinction tightens

### Tier 4 — Functions + closures (the main event)
- First-class functions, calling conventions, environments as first-class values (pointer-to-env captured in closure)
- The "environments are just pointers" moment — parent env chain enables lexical scoping

### Final step — make it a "real language" (file execution)
The gap between a REPL toy and a file-runnable language is smaller than it looks. Do this last, after the language features are in place.

**1. Accept a source file instead of (or alongside) REPL input.**
- Inspect `argc` / `argv[]` in `main`.
- If given a filename: `fopen` + `fread` (or `fstat` + `mmap`) the whole file into a buffer, point the lexer at it, run the statement loop until EOF.
- If no filename: fall back to the existing REPL.
- Roughly 30 lines. Lexer/parser/evaluator are unchanged — they just consume bytes, they don't care where they came from.

**2. "File extension" is convention, not magic.**
Extensions like `.py` / `.c` aren't special to the OS. They're hints for humans and editors. Your language gets an extension the moment you decide on one (e.g. `.lang`) and configure editors to invoke your binary when they see it. The interpreter itself never looks at the filename.

**3. Editor integration / run button.**
Same pattern as the VS Code tasks.json in this project — configure an editor action that shells out to `./learning <filename>`. For the "global feel," copy the binary into `/usr/local/bin/` (on `$PATH`) so any terminal can run `learning program.lang`.

**4. Shebang trick (UNIX only, makes the file self-executable).**
If a `.lang` file's first line is `#!/usr/bin/env learning` and the file is marked executable (`chmod +x`), running `./program.lang` invokes the interpreter with the file as input. No wrapper needed. This is how Python/Ruby/Bash scripts "run themselves" — the OS reads the `#!` line and starts the listed program.

**5. What's actually hard (to know for later).**
At this point the "run a file" plumbing is done. What separates a toy from a real language isn't the file-loading step — it's the *features and ecosystem*: functions, types, stdlib, error messages with source locations, debugger, docs, performance work, package management, community. The interpreter becomes a small nucleus surrounded by years of surrounding work. But once Tier 4 + file loading are done, you've built the engine; the rest is growth.

---

## Key concepts covered so far
- Tagged union pattern for token payloads
- Lexer as a stateful pointer walker
- Parser drives, lexer serves (parser calls next_token)
- Tree shape encodes precedence — parentheses disappear after parsing
- Lower nodes in AST = evaluated first
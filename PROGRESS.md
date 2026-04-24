# Interpreter Progress

## Project: Tree-walking interpreter in C (single file: learning.c)

## Pipeline
```
source text  →  [Lexer]  →  tokens  →  [Parser]  →  AST  →  [Evaluator]  →  value
```

## Project roadmap
- **Tier 1 — Calculator** (complete): integers, `+ - * /`, parens, whitespace, REPL
- **Tier 2 — Variables** (complete): `let NAME = EXPR;`, identifiers, multi-statement lines, session environment
- **Tier 3 — Booleans + if/else** (in progress — lexer + AST types done, parser/eval remaining): comparison ops, control flow
- **Tier 4 — Functions + closures**: first-class functions, lexical scoping, captured environments
- **Type system** (post-Tier-4, staged — see details below): `Value` tagged union, inferred literal types, sized numeric primitives + C-style casts
- **Final step — File execution**: run `.lang` files instead of (or alongside) REPL

---

## Current state snapshot (mid-Tier-3: lexer + AST types done, parser/eval pending)

**Files:** `learning.c` (single-file interpreter), `arena.h`/`arena.c` (bump allocator for AST), `hash.h`/`hash.c` (string→int environment), `.gitignore`, `PROGRESS.md`.

**Compile:** `gcc -Wall -Wextra learning.c arena.c hash.c -o learning`

**`TokenType`** (20 variants): Tier 1–2 tokens + `TOK_IF, TOK_ELSE, TOK_TRUE, TOK_FALSE, TOK_LT, TOK_GT, TOK_EQEQ, TOK_LBRACE, TOK_RBRACE`

**`Token`:** tagged union, `type` + `{ int int_val; char *ident; } value`. `ident` is `strndup`'d by the lexer (leaks per line — known issue).

**`Lexer`:** `char *pos` into source buffer. `next_token` skips whitespace, switches on current char, delegates single-char ops to `casehelper`. Multi-char `==` uses a new `view_next(l)` lookahead helper + `casehelper2` (advances 2 chars). Identifier branch recognizes keywords via length+strncmp: `let`, `if`, `else`, `true`, `false`.

**`OpType`:** `OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_LT, OP_GT, OP_EQEQ` — last three are Tier 3 additions, not yet wired into eval.

**`NodeType`:** `NODE_INT, NODE_BINOP, NODE_IDENT, NODE_LET, NODE_BOOL, NODE_IF, NODE_BLOCK`

**`Node`:** tagged union; top-level fields are `type` + `struct Node *next` (used for statement chains inside blocks — NULL elsewhere). `uni` has arms for int_value, bool_val, binop (op + left/right), ident_name, let (name + value subtree), if_stmt (cond + then_branch + else_branch — else nullable), and block (`first` pointing to the head of a `next`-linked statement chain).

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

## Tier 3 — Booleans + if/else (in progress)

### Design decisions (locked)
- `if` is a **statement**, not an expression — doesn't produce a value
- **Braces mandatory** around branches: `if (cond) { body }`
- **Else is optional** — bare `if` allowed, NODE_IF's `else_branch` is nullable
- **Bools treated as ints** (0/1) — no new Value type (deferred to post-Tier-4 type system)
- **Comparison ops don't chain** — `a < b < c` will be a parse error (one comparison, no repeat)
- **Comparison precedence** lower than arithmetic — `x + 1 > 3` parses as `(x+1) > 3`

### Grammar additions
```
statement  → 'let' IDENT '=' comparison ';'
           | 'if' '(' comparison ')' block ('else' block)?
           | comparison ';'
block      → '{' statement* '}'
comparison → expr (('<' | '>' | '==') expr)?
```

(`comparison` replaces the callers that used to call `parse_expr` directly — `parse_let` RHS and expr-stmts.)

### Lexer — complete
- 9 new `TokenType`s (if/else/true/false/lt/gt/eqeq/lbrace/rbrace) added
- `view_next(Lexer *l)` helper — peeks one char past `l->pos`, with EOF safety (returns `\0` if current is already null)
- `casehelper2` — 2-char version of casehelper, advances pos by 2
- `=` case: uses `view_next` to choose between TOK_EQ (single-char `casehelper`) and TOK_EQEQ (`casehelper2`)
- Keyword recognition extended in the identifier branch: `if`, `else`, `true`, `false` checked alongside `let` via length + `strncmp` chain
- `print_token` extended with 9 new cases
- Verified on `"if (x == 5) { true } else { false }"` — all tokens produced in order, multi-char `==` correctly emits EQEQ

### AST types — complete
- `NodeType` grew `NODE_BOOL, NODE_IF, NODE_BLOCK`
- `OpType` grew `OP_LT, OP_GT, OP_EQEQ` (comparisons reuse NODE_BINOP — no new node type)
- `Node` gained top-level `struct Node *next` field for block statement chains (Option 1 representation — simplest, 8 bytes overhead per node)
- New union arms:
  - `int bool_val` for NODE_BOOL (0/1, treated as int for now)
  - `struct { Node *cond, *then_branch, *else_branch; } if_stmt` — else_branch nullable
  - `struct Node *first` for NODE_BLOCK — points at head of `next`-linked chain

### What's next (Tier 3 pickup)

1. **`parse_comparison`** — calls `parse_expr`, then optionally consumes one comparison operator (TOK_LT/TOK_GT/TOK_EQEQ) and another `parse_expr`, builds NODE_BINOP with the right OpType. Single comparison only, no chaining. Then: find every caller of `parse_expr` that should accept comparisons (parse_let RHS, parse_statement expr-stmt path) and swap to `parse_comparison`. `parse_expr` itself stays unchanged.

2. **`parse_block`** — expects `{`, loops `parse_statement` while curr isn't `}` (guard against EOF), chains results via `next` using head/tail locals, expects `}`. Returns NODE_BLOCK with `first = head`.

3. **`parse_if`** — advance past `if`, expect `(`, `parse_comparison` for cond, expect `)`, `parse_block` for then, optionally if next is `else` advance and `parse_block` for else (else nullable → else_branch stays NULL if absent). Build NODE_IF. No trailing `;` — if is brace-delimited.

4. **`parse_statement` wire-up:** add a TOK_IF branch at the top that delegates to `parse_if`. Update the expr-stmt path to use `parse_comparison` instead of `parse_expr`.

5. **`parse_factor` extension:** add cases for TOK_TRUE → NODE_BOOL with `bool_val = 1`, TOK_FALSE → NODE_BOOL with `bool_val = 0`. Both advance + return.

6. **Forward declarations** — add `parse_comparison`, `parse_block`, `parse_if` to the top-of-parser block.

7. **Evaluator additions:**
   - NODE_BOOL → return `bool_val`
   - NODE_IF → eval cond; if non-zero, eval then_branch; else if else_branch != NULL, eval it; return 0 (statement, no meaningful value)
   - NODE_BLOCK → walk the chain from `first`, eval each via `next`, return the last value (or 0 if empty)
   - BINOP switch gains OP_LT / OP_GT / OP_EQEQ → return 1 or 0 from the comparison
   - Also update `print_ast`'s BINOP switch for the 3 new OpTypes (currently compiler-warning about unhandled switch cases)

8. **Main loop:** the existing structure (`parse_statement` loop, print if not NODE_LET) mostly works. Probably want to also suppress printing for NODE_IF and NODE_BLOCK — they're statements that don't produce useful values either.

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

### Type system — Rust/Zig-lite primitives (staged, post-Tier-4)

**The dream.** C-style defaults, Rust-style sized primitives with explicit casts:
```
let x = 1;             // default int (i32)
let y = 3.14;          // default float (f64)
let c = 'a';           // char (u8)
let b = true;          // bool
let small = (u8) 1;    // explicit narrow
let half = (f16) 3.14; // explicit float width
```

**Target primitive types:** `u8 u16 u32 u64 i8 i16 i32 i64 f16 f32 f64 bool char`

**Defaults (C-style):**
- Integer literals → `i32` (no cast required, matches C's `int`)
- Float literals → `f64`
- Char literals → `u8`
- `true` / `false` → `bool`
- Mixed-type operations use **implicit promotion** (widen operands to the wider type, then op) — e.g. `(u8) 1 + 5` → `i32` holding 6

**Why this is the right model for a teaching language:**
- No cast clutter for the 95% case (`let x = 1;` just works)
- Explicit widths available when needed
- Promotion rules match C — predictable, familiar
- Cast syntax `(TYPE) EXPR` is C-classic

**Staging plan:**

**Stage B — Generic Value tagged union (1–2 days):**
- `Value` tagged union: tag + union with arms for `int64`, `double`, `bool`, `char` (just 4 types, no size variants yet)
- `eval` returns `Value` instead of `int`; hashmap stores `Value`
- Lexer extensions: float literals (digit-loop handling `.`), char literals (`'a'`), `true`/`false` keywords
- Operators gain type-dispatch: promote to common type, then operate
- `print_value(Value)` helper switches on tag — `%d`/`%f`/`%c`/`"true"|"false"`
- Unlocks: `let x = 'a'; let y = 3.14; let z = true;` — three of the four dream features

**Stage C — Sized numerics + cast operator (1–2 days):**
- Expand `Value`'s tag enum to cover all 13 primitive types; storage stays compact (signed ints share an `int64_t`, unsigned share a `uint64_t`, floats share a `double` — tag tells you how to interpret)
- Lexer: add tokens for type names (`TOK_U8`, `TOK_I32`, `TOK_F64`, ...) — lex as identifier, recognize as keyword (same trick as `let`)
- **Parser — the interesting part:** cast syntax needs **two-token lookahead**. When `parse_factor` sees `(`, it has to decide between `(expr)` and `(type) expr`. Requires extending `Parser` with a second lookahead slot (e.g. `Token next` alongside `Token curr`), plus a `peek_ahead` helper. First time the parser needs >1 lookahead.
- New node: `NODE_CAST` wrapping an expression + target type
- Evaluator: cast logic — widening is safe, narrowing truncates (or errors — decide)
- Unlocks: full dream — `let x = (u8) 1; let y = (f16) 3.14;`

**Stage D — Static type checking (optional, days to weeks):**
- A type-checking pass between parser and evaluator; catches `true + 5` at parse time instead of runtime
- Type annotations on let: `let x: u16 = 5;`
- Generic inference ("figure out `x`'s type from how it's used") — simpler languages use unification (Hindley-Milner is the canonical algorithm)
- Probably beyond the natural stopping point for this project; noted for completeness

**Why do types *after* Tier 4:**
- Types are orthogonal to control flow and functions — retrofit later without rewriting either
- Don't mix "what's a closure?" (hard) with "how do values interact?" (tedious) — one at a time
- Tier 4's `Value`-as-function-pointer design will tell you exactly what the Value union needs to look like, so designing it post-Tier-4 avoids redesign

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
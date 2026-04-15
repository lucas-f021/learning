# Session handoff — tree-walking interpreter project

## Context for the next Claude session

Paste or reference this file when starting a new session so Claude picks up where we left off.

## Collaboration preference (CRITICAL — user emphasized this explicitly)

**NEVER give the user code. This is a real learning experience.**

Direct quote from the user: *"I want you to NEVER give me the code. This is gonna be a real learning experience. If I ask a question, I want you to teach me the theory rather than the explicit answer. You can point me in the right direction, but make every question a teaching moment."*

Concrete rules:

- **No paste-ready code for anything the user is building.** Not even "small" snippets or "just this one line." Not even when the user asks for it directly. The user has pre-consented to being told no.
- **Every question is a teaching moment.** When the user asks "how do I do X?", do not answer with the mechanical answer. Answer with the *theory* — what X is, why it works, what the tradeoffs are, what the shape of the solution looks like. Then let them write it.
- **Hints over answers.** You can describe the shape of the solution ("you'll need a variable of type T, initialized from expression E"), point to the relevant concept, give analogies. You cannot write the expression itself.
- **If they're stuck, go deeper on the theory, not closer to the code.** Confusion means the mental model isn't solid. Re-explain the mechanism, draw a picture, walk through a concrete example with values. Don't shortcut to "here's the code."
- **Reviewing their code is encouraged.** After they write something, read it, diff it, call out bugs precisely with line numbers and *why* they're bugs. Corrections pointing at their line are fine — rewriting the line for them is not.

Exception that still holds: **test scaffolding / `main` / I/O plumbing is fair game.** The user has learned the technique by building it; tests that exercise it are not the learning target. If in doubt, ask.

Meta-rule: if you find yourself about to type ` ```c ` (or any code block with code the user will paste), stop. Convert it to a description of what that code would do and why.

## Prior work in this directory

Both complete, working, ASan-clean. Reference them if useful but the interpreter is a fresh project.

- `customhash.c` — teaching-grade hashmap in C. String keys, int values, open addressing with linear probing, FNV-1a, power-of-2 capacity, 70% load-factor resize. See `HANDOFF.md` for full notes.
- `arenaalloc.c` — bump/arena allocator in C. `Arena` struct with `start`/`nxt_byte`/`end` (all `void *`). 8-byte alignment via `(n + 7) & ~(uintptr_t)7` trick. Overflow returns NULL. `arena_reset` sets `nxt_byte = start`. Covered the "why arenas" motivation thoroughly — speed, lifetime management, fragmentation, cache locality, bulk free.

## New project: tree-walking interpreter

### Scope plan agreed on

Climb in tiers, each teaching a distinct concept. Don't skip tiers — debugging across three undebuggable layers at once is brutal for a learner.

1. **Tier 1 — Calculator.** Integers, `+ - * /`, parentheses. Full pipeline (lexer → parser → evaluator) end-to-end. ~100 lines.
2. **Tier 2 — Variables.** `let x = 5;`, identifiers, statements vs expressions, environment (name → value map — the hashmap could shine here if user writes in C).
3. **Tier 3 lite — Booleans + if/else.** Control flow.
4. **Tier 4 — Functions + closures.** The payoff tier. "Environments are just pointers" is the rewiring moment.

### The three phases (reference for explanations)

```
source text  →  [Lexer]     →  tokens
tokens       →  [Parser]    →  AST
AST          →  [Evaluator] →  value
```

- Lexer: mechanical categorization of characters into tokens. No meaning.
- Parser: recursive descent, encodes operator precedence in tree shape.
- Evaluator: recursive walk of the tree at runtime.

### OPEN QUESTION — resolve first

**What language is the user writing the interpreter *in*?** Not answered before the session switched. Options discussed:

- **C** — consistent with prior projects, full systems experience, manual memory. Arena allocator from this project would be genuinely useful for AST node allocation. Slower to prototype.
- **Python** — 1:1 with concepts, fast to iterate, focus stays on *ideas* rather than memory management. Classic language for this project.
- **Other** — Go, Rust, etc. also fine.

Ask the user before starting. This choice shapes every hint.

### Teaching approach notes (from prior sessions)

- User responds well to: visual diagrams (ASCII art of memory layouts, tree structures), bit-level walk-throughs, explicit "why this instead of that" design choices, multiple-choice questions when a decision point arises.
- User gets confused by: terminology drift (e.g. I said "base" while their field was named "start" — caused real confusion). Pick the user's names and stick with them.
- User frequently shares IDE-opened file state — read it before commenting rather than asking.
- User often asks "check my code" — read the file fresh each time, diff against last-known state, call out bugs precisely with line numbers and *why*.
- User's C has some recurring gotchas to watch for: `return;` in non-void functions, `uint8_t` vs `uint8_t *` (the star matters), declaration vs cast syntax confusion, `sizeof(ptr)` vs `sizeof(*ptr)`.

### Suggested first move

After confirming implementation language:

1. Get alignment on what Tier 1 supports *exactly* (integer literals only, or also negative numbers? unary minus? multi-digit?).
2. Start with the lexer — simplest phase, immediate feedback via printing token stream.
3. Ask the user to enumerate the token types they'll need before writing any code.

## File locations

All projects in `/Users/lucasfrolio/vibecoding/`. Prior handoff in `HANDOFF.md`, this one in `INTERPRETER_HANDOFF.md`.

# Tree-Walking Interpreter Visualizer (Tier 1)

A 3D, browser-based visualizer that animates the recursive parsing and
evaluation of arithmetic expressions — mirroring the Tier 1 lexer/parser/eval
pipeline of the C interpreter in `../learning/`.

## What it shows

- **Source pane (left):** the input expression with token-level highlighting.
  The "current" token is yellow; consumed tokens fade out.
- **3D tree (center):** the AST as it gets built, one node at a time.
  Blue boxes = `INT` leaves. Orange boxes = `BINOP` operator nodes.
  Lines grow from parent to child as the parser connects them.
- **Call stack (right):** parser function frames — `parse_expr`, `parse_term`,
  `parse_factor` — push when entered and pop when they return. Each frame
  shows the current lookahead token.
- **Eval mode:** after parse finishes, click "→ Eval Mode" to replay with the
  evaluator. Each node is highlighted yellow as eval enters it; its computed
  value pops out below as eval returns.

## Running locally

ES modules don't work over `file://` — you need a tiny local server.

```sh
cd visualizer
python3 -m http.server 8000
# then open http://localhost:8000/ in your browser
```

(Or `npx serve` if you have node, or any other static server.)

## Controls

- **Run** — re-parse the expression in the input box
- **Play / Pause** — auto-step through events
- **Step ⏭** — advance one event manually
- **⏮ Step Back** — replay from the start up to one event before current
  (no real undo — replays from scratch)
- **Speed** slider — left = slow, right = fast
- **↻ Restart** — re-run the current mode from the beginning
- **→ Eval Mode** — switch from parse to eval (enabled after parse finishes)

## What's intentionally simple

- Tier 1 only: `+ - * /`, parens, integers, unary minus, whitespace.
- No variables, no `if`, no blocks. The C interpreter does all of those at
  Tier 2/3 — adding them here would muddy the recursion lesson.
- Step Back replays from scratch instead of true undoing — fine for short
  expressions, lazy for long ones.
- Layout is computed once from the final AST (positions are pre-known when
  animation begins).

## Files

- `index.html` — markup, importmap pointing at Three.js on unpkg
- `style.css` — dark, minimal educational theme
- `interpreter.js` — Tier 1 lexer + parser + evaluator with event recording
- `layout.js` — bottom-up width-based AST positioning
- `main.js` — Three.js scene, animator, UI wiring

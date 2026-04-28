// interpreter.js
// Tier 1 lexer + parser + evaluator that record an event stream
// suitable for animated playback. Mirrors the C interpreter's structure.

export const TOK = {
  INT: 'INT', PLUS: 'PLUS', MINUS: 'MINUS', STAR: 'STAR', SLASH: 'SLASH',
  LPAREN: 'LPAREN', RPAREN: 'RPAREN', EOF: 'EOF',
};

export function tokenLabel(t) {
  if (!t) return 'EOF';
  if (t.type === 'INT') return `INT(${t.value})`;
  return t.type;
}

export function tokenize(source) {
  const tokens = [];
  let i = 0;
  while (i < source.length) {
    const c = source[i];
    if (/\s/.test(c)) { i++; continue; }
    if (/\d/.test(c)) {
      let j = i;
      while (j < source.length && /\d/.test(source[j])) j++;
      tokens.push({
        type: TOK.INT,
        value: parseInt(source.slice(i, j), 10),
        start: i, end: j,
      });
      i = j;
      continue;
    }
    const single = {
      '+': TOK.PLUS, '-': TOK.MINUS, '*': TOK.STAR, '/': TOK.SLASH,
      '(': TOK.LPAREN, ')': TOK.RPAREN,
    }[c];
    if (single) {
      tokens.push({ type: single, start: i, end: i + 1 });
      i++;
      continue;
    }
    throw new Error(`Unknown character "${c}" at position ${i}`);
  }
  tokens.push({ type: TOK.EOF, start: i, end: i });
  return tokens;
}

let nodeIdCounter = 0;

export function parseWithEvents(tokens) {
  nodeIdCounter = 0;
  const events = [];
  let pos = 0;

  const curr = () => tokens[pos];

  const newNode = (data) => ({ id: `n${nodeIdCounter++}`, ...data });

  const enter = (fn) => events.push({ kind: 'enter', fn, token: curr() });
  const exit = (fn, returnedId) => events.push({ kind: 'exit', fn, returnedId });
  const advance = () => {
    const t = tokens[pos];
    events.push({ kind: 'consume', tokenIdx: pos });
    pos++;
    events.push({ kind: 'update_top_token', token: tokens[pos] });
    return t;
  };
  const emitCreate = (node) => events.push({ kind: 'create', node });
  const emitConnect = (parent, child, role) =>
    events.push({ kind: 'connect', parentId: parent.id, childId: child.id, role });

  function parse_factor() {
    enter('parse_factor');
    const t = curr();
    let result;

    if (t.type === TOK.INT) {
      result = newNode({ type: 'INT', value: t.value });
      emitCreate(result);
      advance();
    } else if (t.type === TOK.LPAREN) {
      advance();
      result = parse_expr();
      if (curr().type !== TOK.RPAREN) {
        throw new Error(`Expected ')' but got ${tokenLabel(curr())}`);
      }
      advance();
    } else if (t.type === TOK.MINUS) {
      // Unary minus: synthesize 0 - operand
      advance();
      const operand = parse_factor();
      const zero = newNode({ type: 'INT', value: 0 });
      emitCreate(zero);
      result = newNode({ type: 'BINOP', op: '-', left: zero, right: operand });
      emitCreate(result);
      emitConnect(result, zero, 'left');
      emitConnect(result, operand, 'right');
    } else {
      throw new Error(`Unexpected token: ${tokenLabel(t)}`);
    }
    exit('parse_factor', result.id);
    return result;
  }

  function parse_term() {
    enter('parse_term');
    let left = parse_factor();
    while (curr().type === TOK.STAR || curr().type === TOK.SLASH) {
      const op = curr().type === TOK.STAR ? '*' : '/';
      advance();
      const right = parse_factor();
      const node = newNode({ type: 'BINOP', op, left, right });
      emitCreate(node);
      emitConnect(node, left, 'left');
      emitConnect(node, right, 'right');
      left = node;
    }
    exit('parse_term', left.id);
    return left;
  }

  function parse_expr() {
    enter('parse_expr');
    let left = parse_term();
    while (curr().type === TOK.PLUS || curr().type === TOK.MINUS) {
      const op = curr().type === TOK.PLUS ? '+' : '-';
      advance();
      const right = parse_term();
      const node = newNode({ type: 'BINOP', op, left, right });
      emitCreate(node);
      emitConnect(node, left, 'left');
      emitConnect(node, right, 'right');
      left = node;
    }
    exit('parse_expr', left.id);
    return left;
  }

  const ast = parse_expr();
  if (curr().type !== TOK.EOF) {
    throw new Error(`Unexpected token after expression: ${tokenLabel(curr())}`);
  }
  return { ast, events };
}

export function evalWithEvents(ast) {
  const events = [];

  function ev(node) {
    events.push({ kind: 'enter', id: node.id, nodeType: node.type, op: node.op, value: node.value });
    let value;
    if (node.type === 'INT') {
      value = node.value;
    } else {
      const l = ev(node.left);
      const r = ev(node.right);
      switch (node.op) {
        case '+': value = l + r; break;
        case '-': value = l - r; break;
        case '*': value = l * r; break;
        case '/': value = r === 0 ? 0 : Math.trunc(l / r); break;
      }
    }
    events.push({ kind: 'exit', id: node.id, value });
    return value;
  }

  const finalValue = ev(ast);
  return { events, finalValue };
}

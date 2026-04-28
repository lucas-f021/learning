// layout.js
// Compute 3D positions for AST nodes. Bottom-up width-based layout:
// each leaf takes one slot, each binop sits centered above its children.

const X_SPACING = 1.9;
const Y_SPACING = 1.7;

export function layoutTree(root) {
  const positions = {};
  let nextLeafX = 0;

  function assign(node, depth) {
    if (node.type === 'INT') {
      positions[node.id] = {
        x: nextLeafX * X_SPACING,
        y: -depth * Y_SPACING,
        z: 0,
      };
      nextLeafX++;
      return;
    }
    assign(node.left, depth + 1);
    assign(node.right, depth + 1);
    const lx = positions[node.left.id].x;
    const rx = positions[node.right.id].x;
    positions[node.id] = {
      x: (lx + rx) / 2,
      y: -depth * Y_SPACING,
      z: 0,
    };
  }

  assign(root, 0);

  // Center the entire tree horizontally around x = 0.
  const xs = Object.values(positions).map(p => p.x);
  const cx = (Math.min(...xs) + Math.max(...xs)) / 2;
  for (const id in positions) {
    positions[id].x -= cx;
  }
  return positions;
}

export function boundingBox(positions) {
  const xs = Object.values(positions).map(p => p.x);
  const ys = Object.values(positions).map(p => p.y);
  return {
    minX: Math.min(...xs),
    maxX: Math.max(...xs),
    minY: Math.min(...ys),
    maxY: Math.max(...ys),
  };
}

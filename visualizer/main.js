// main.js
// Three.js scene + animator + UI wiring.

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { CSS2DRenderer, CSS2DObject } from 'three/addons/renderers/CSS2DRenderer.js';
import {
  tokenize, tokenLabel, parseWithEvents, evalWithEvents,
} from './interpreter.js';
import { layoutTree, boundingBox } from './layout.js';

// ===== Three.js setup =====
const canvas = document.getElementById('canvas');
const labelContainer = document.getElementById('label-container');

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x0e1116);

const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.setPixelRatio(window.devicePixelRatio);

const labelRenderer = new CSS2DRenderer();
labelRenderer.domElement.style.position = 'absolute';
labelRenderer.domElement.style.top = '0';
labelRenderer.domElement.style.left = '0';
labelRenderer.domElement.style.width = '100%';
labelRenderer.domElement.style.height = '100%';
labelRenderer.domElement.style.pointerEvents = 'none';
labelContainer.appendChild(labelRenderer.domElement);

const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 200);
camera.position.set(0, -3, 12);

const controls = new OrbitControls(camera, canvas);
controls.target.set(0, -3, 0);
controls.enableDamping = true;
controls.dampingFactor = 0.1;
controls.update();

scene.add(new THREE.AmbientLight(0xffffff, 0.7));
const dir = new THREE.DirectionalLight(0xffffff, 0.65);
dir.position.set(5, 10, 7);
scene.add(dir);

function resize() {
  const rect = canvas.parentElement.getBoundingClientRect();
  camera.aspect = rect.width / rect.height;
  camera.updateProjectionMatrix();
  renderer.setSize(rect.width, rect.height);
  labelRenderer.setSize(rect.width, rect.height);
}
window.addEventListener('resize', resize);
resize();

// ===== Tree group =====
const NODE_COLOR = { INT: 0x4a90e2, BINOP: 0xe27a4a };
const treeGroup = new THREE.Group();
scene.add(treeGroup);

let nodeMeshes = {};   // node.id -> THREE.Mesh
let edgeMeshes = [];   // array of THREE.Line

function disposeMaterial(obj) {
  if (obj.material) {
    if (Array.isArray(obj.material)) obj.material.forEach(m => m.dispose());
    else obj.material.dispose();
  }
  if (obj.geometry) obj.geometry.dispose();
}

function clearScene() {
  // Dispose CSS2DObjects too (they're children of meshes)
  while (treeGroup.children.length > 0) {
    const child = treeGroup.children[0];
    // remove any CSS2DObject children (DOM elements)
    while (child.children.length > 0) {
      const grand = child.children[0];
      if (grand.element && grand.element.parentNode) {
        grand.element.parentNode.removeChild(grand.element);
      }
      child.remove(grand);
    }
    disposeMaterial(child);
    treeGroup.remove(child);
  }
  nodeMeshes = {};
  edgeMeshes = [];
}

function createNodeMesh(node, position) {
  const isInt = node.type === 'INT';
  const w = isInt ? 1.5 : 1.7;
  const geo = new THREE.BoxGeometry(w, 0.75, 0.75);
  const mat = new THREE.MeshStandardMaterial({
    color: NODE_COLOR[node.type],
    roughness: 0.55,
    metalness: 0.1,
  });
  const mesh = new THREE.Mesh(geo, mat);
  mesh.position.set(position.x, position.y, position.z);

  const labelEl = document.createElement('div');
  labelEl.className = 'node-label';
  labelEl.textContent = isInt ? `INT(${node.value})` : `BINOP ${node.op}`;
  const label = new CSS2DObject(labelEl);
  label.position.set(0, 0.6, 0);
  mesh.add(label);

  const valueEl = document.createElement('div');
  valueEl.className = 'node-value';
  valueEl.textContent = '';
  valueEl.style.display = 'none';
  const valueLabel = new CSS2DObject(valueEl);
  valueLabel.position.set(0, -0.55, 0);
  mesh.add(valueLabel);

  mesh.userData = {
    valueEl,
    material: mat,
    targetScale: 1,
  };

  // Start invisible, animate scale-in
  mesh.scale.setScalar(0);

  treeGroup.add(mesh);
  nodeMeshes[node.id] = mesh;
  return mesh;
}

function createEdgeMesh(parentId, childId) {
  const parent = nodeMeshes[parentId];
  const child = nodeMeshes[childId];
  if (!parent || !child) return;
  const start = parent.position.clone();
  const end = child.position.clone();
  const geo = new THREE.BufferGeometry().setFromPoints([start, start.clone()]);
  const mat = new THREE.LineBasicMaterial({ color: 0x6a7280, linewidth: 2 });
  const line = new THREE.Line(geo, mat);
  line.userData = { start, end, progress: 0 };
  treeGroup.add(line);
  edgeMeshes.push(line);
}

function highlightNode(id, on) {
  const mesh = nodeMeshes[id];
  if (!mesh) return;
  if (on) {
    mesh.userData.material.emissive = new THREE.Color(0xffe066);
    mesh.userData.material.emissiveIntensity = 0.55;
  } else {
    mesh.userData.material.emissive = new THREE.Color(0x000000);
    mesh.userData.material.emissiveIntensity = 0;
  }
}

function showNodeValue(id, value) {
  const mesh = nodeMeshes[id];
  if (!mesh) return;
  mesh.userData.valueEl.style.display = 'inline-block';
  mesh.userData.valueEl.textContent = value;
}

function clearNodeValues() {
  for (const id in nodeMeshes) {
    const m = nodeMeshes[id];
    m.userData.valueEl.style.display = 'none';
    m.userData.valueEl.textContent = '';
    highlightNode(id, false);
  }
}

// ===== Stack & source UI =====
const stackContainer = document.getElementById('stack-container');
const sourceContainer = document.getElementById('source-text');
const errorDisplay = document.getElementById('error-display');
const resultDisplay = document.getElementById('result-display');
const modeDisplay = document.getElementById('mode-display');
const evalModeBtn = document.getElementById('eval-mode-btn');
const progressDisplay = document.getElementById('progress-display');

let stackElements = [];
let sourceTokens = [];

function clearStack() {
  stackContainer.innerHTML = '';
  stackElements = [];
}

function pushStackFrame(fn, info) {
  const el = document.createElement('div');
  el.className = 'stack-frame entering';
  el.innerHTML =
    `<div class="fn">${fn}</div>` +
    `<div class="curr">${info}</div>`;
  stackContainer.appendChild(el);
  stackElements.push(el);
  // Force reflow then remove the entering class for animation
  void el.offsetHeight;
  el.classList.remove('entering');
}

function popStackFrame() {
  const el = stackElements.pop();
  if (!el) return;
  el.classList.add('leaving');
  setTimeout(() => el.remove(), 300);
}

function updateTopFrameToken(token) {
  if (stackElements.length === 0) return;
  const top = stackElements[stackElements.length - 1];
  const cur = top.querySelector('.curr');
  if (cur) cur.textContent = `curr: ${tokenLabel(token)}`;
}

function renderSource(tokens, sourceText) {
  sourceContainer.innerHTML = '';
  sourceTokens = [];
  let lastEnd = 0;
  for (const t of tokens) {
    if (t.type === 'EOF') continue;
    if (t.start > lastEnd) {
      sourceContainer.append(sourceText.slice(lastEnd, t.start));
    }
    const span = document.createElement('span');
    span.className = 'token';
    span.textContent = sourceText.slice(t.start, t.end);
    sourceContainer.appendChild(span);
    sourceTokens.push({ ...t, element: span });
    lastEnd = t.end;
  }
  if (sourceTokens.length > 0) {
    sourceTokens[0].element.classList.add('current');
  }
}

function consumeSourceToken(idx) {
  if (idx >= 0 && idx < sourceTokens.length) {
    sourceTokens[idx].element.classList.add('consumed');
    sourceTokens[idx].element.classList.remove('current');
  }
  if (idx + 1 < sourceTokens.length) {
    sourceTokens[idx + 1].element.classList.add('current');
  }
}

function resetSourceHighlights() {
  for (const t of sourceTokens) {
    t.element.classList.remove('current', 'consumed');
  }
  if (sourceTokens.length > 0) {
    sourceTokens[0].element.classList.add('current');
  }
}

// ===== Animator =====
let parseEventsCache = [];
let evalEventsCache = [];
let astCache = null;
let positionsCache = null;
let astNodesById = {};

let events = [];
let eventIdx = 0;
let isPlaying = false;
let speed = 600; // ms per step
let lastStepTime = 0;
let mode = 'parse';
let consumedTokenCount = 0;

function indexAst(root) {
  const out = {};
  function walk(n) {
    out[n.id] = n;
    if (n.type === 'BINOP') { walk(n.left); walk(n.right); }
  }
  walk(root);
  return out;
}

function nodeDescription(node) {
  if (!node) return '';
  if (node.type === 'INT') return `INT(${node.value})`;
  return `BINOP ${node.op}`;
}

function processEvent(e) {
  if (mode === 'parse') {
    switch (e.kind) {
      case 'enter':
        pushStackFrame(e.fn, `curr: ${tokenLabel(e.token)}`);
        break;
      case 'consume':
        consumeSourceToken(consumedTokenCount);
        consumedTokenCount++;
        break;
      case 'update_top_token':
        updateTopFrameToken(e.token);
        break;
      case 'create':
        if (positionsCache[e.node.id]) {
          createNodeMesh(e.node, positionsCache[e.node.id]);
        }
        break;
      case 'connect':
        createEdgeMesh(e.parentId, e.childId);
        break;
      case 'exit':
        popStackFrame();
        break;
    }
  } else {
    // Eval mode
    switch (e.kind) {
      case 'enter': {
        const node = astNodesById[e.id];
        highlightNode(e.id, true);
        pushStackFrame('eval', `node: ${nodeDescription(node)}`);
        break;
      }
      case 'exit':
        showNodeValue(e.id, e.value);
        highlightNode(e.id, false);
        popStackFrame();
        break;
    }
  }
  updateProgress();
}

function step() {
  if (eventIdx >= events.length) {
    isPlaying = false;
    if (mode === 'parse' && parseEventsCache.length > 0) {
      evalModeBtn.disabled = false;
    }
    return false;
  }
  processEvent(events[eventIdx]);
  eventIdx++;
  return true;
}

function updateProgress() {
  progressDisplay.textContent = `${eventIdx} / ${events.length}`;
}

// ===== Animation loop =====
function tick(t) {
  if (isPlaying && t - lastStepTime > speed) {
    if (!step()) {
      isPlaying = false;
      updatePlayPauseBtn();
    }
    lastStepTime = t;
  }

  // Animate node scale-in
  for (const id in nodeMeshes) {
    const mesh = nodeMeshes[id];
    const target = mesh.userData.targetScale || 1;
    if (mesh.scale.x < target) {
      const next = Math.min(target, mesh.scale.x + 0.18);
      mesh.scale.setScalar(next);
    }
  }

  // Animate edge growth
  for (const e of edgeMeshes) {
    if (e.userData.progress < 1) {
      e.userData.progress = Math.min(1, e.userData.progress + 0.08);
      const cur = e.userData.start.clone().lerp(e.userData.end, e.userData.progress);
      e.geometry.setFromPoints([e.userData.start, cur]);
    }
  }

  controls.update();
  renderer.render(scene, camera);
  labelRenderer.render(scene, camera);
  requestAnimationFrame(tick);
}
requestAnimationFrame(tick);

// ===== UI wiring =====
const exprInput = document.getElementById('expr-input');
const runBtn = document.getElementById('run-btn');
const playPauseBtn = document.getElementById('play-pause-btn');
const stepFwdBtn = document.getElementById('step-fwd-btn');
const stepBackBtn = document.getElementById('step-back-btn');
const restartBtn = document.getElementById('restart-btn');
const speedSlider = document.getElementById('speed-slider');

function updatePlayPauseBtn() {
  playPauseBtn.textContent = isPlaying ? '⏸ Pause' : '▶ Play';
}

speedSlider.addEventListener('input', () => {
  // Slider is delay-ish; invert so right side feels faster.
  speed = 2100 - parseInt(speedSlider.value, 10);
});
speed = 2100 - parseInt(speedSlider.value, 10);

playPauseBtn.addEventListener('click', () => {
  isPlaying = !isPlaying;
  if (isPlaying) lastStepTime = performance.now() - speed;
  updatePlayPauseBtn();
});

stepFwdBtn.addEventListener('click', () => {
  isPlaying = false;
  step();
  updatePlayPauseBtn();
});

stepBackBtn.addEventListener('click', () => {
  if (eventIdx <= 0) return;
  const target = eventIdx - 1;
  // Restart and replay up to target without auto-play
  isPlaying = false;
  resetForReplay();
  for (let i = 0; i < target; i++) step();
  updatePlayPauseBtn();
});

restartBtn.addEventListener('click', () => {
  isPlaying = false;
  resetForReplay();
  isPlaying = true;
  lastStepTime = performance.now() - speed;
  updatePlayPauseBtn();
});

evalModeBtn.addEventListener('click', () => {
  mode = 'eval';
  modeDisplay.textContent = 'Eval';
  events = evalEventsCache;
  eventIdx = 0;
  clearStack();
  clearNodeValues();
  evalModeBtn.disabled = true;
  isPlaying = true;
  lastStepTime = performance.now() - speed;
  updatePlayPauseBtn();
  updateProgress();
});

function resetForReplay() {
  // Returns to the start of the current mode (parse or eval)
  if (mode === 'parse') {
    clearScene();
    clearStack();
    consumedTokenCount = 0;
    resetSourceHighlights();
    events = parseEventsCache;
    evalModeBtn.disabled = true;
  } else {
    // Eval mode: keep tree but clear values + stack
    clearStack();
    clearNodeValues();
    events = evalEventsCache;
  }
  eventIdx = 0;
  updateProgress();
}

function fitCameraToTree() {
  if (!positionsCache) return;
  const bb = boundingBox(positionsCache);
  const cx = (bb.minX + bb.maxX) / 2;
  const cy = (bb.minY + bb.maxY) / 2;
  const width = bb.maxX - bb.minX + 4;
  const height = bb.maxY - bb.minY + 4;
  const dist = Math.max(width, height) * 1.3 + 4;
  camera.position.set(cx, cy, dist);
  controls.target.set(cx, cy, 0);
  controls.update();
}

function runExpression() {
  errorDisplay.textContent = '';
  const source = exprInput.value;
  if (!source.trim()) {
    errorDisplay.textContent = 'Enter an expression.';
    return;
  }
  try {
    const tokens = tokenize(source);
    renderSource(tokens, source);

    const { ast, events: parseEvents } = parseWithEvents(tokens);
    const positions = layoutTree(ast);

    parseEventsCache = parseEvents;
    astCache = ast;
    positionsCache = positions;
    astNodesById = indexAst(ast);

    const { events: evalEvents, finalValue } = evalWithEvents(ast);
    evalEventsCache = evalEvents;

    resultDisplay.textContent = `${finalValue}`;

    clearScene();
    clearStack();
    consumedTokenCount = 0;
    mode = 'parse';
    modeDisplay.textContent = 'Parse';
    events = parseEventsCache;
    eventIdx = 0;
    evalModeBtn.disabled = true;

    fitCameraToTree();

    isPlaying = true;
    lastStepTime = performance.now() - speed;
    updatePlayPauseBtn();
    updateProgress();
  } catch (err) {
    errorDisplay.textContent = err.message;
  }
}

runBtn.addEventListener('click', runExpression);
exprInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') runExpression();
});

// Auto-run with default expression on load
runExpression();

import * as shader from "./shader.js"

const animating = true;
const nAllPts = 128;
const dispw = 720, disph = 576;
const elmCanvas = document.getElementById('webgl-canvas');
// const colors = [[0.60, 0.65, 1.00], [0.75, 0.71, 1.00], [0.05, 0.96, 1.00]];
const colors = [[0.4, 0.612, 0.98], [0.31, 0.09, 0.529], [0.984, 0.475, 0.235]];

let pts = [];
let clrIxFrom = 0, clrIxTo = 1, clrInter = 0;

await init();

async function init() {
  elmCanvas.width = dispw;
  elmCanvas.height = disph;

  for (let i = 0; i < nAllPts; ++i)
    pts.push(0, 0, 0, i / nAllPts);
  shader.init(elmCanvas, pts);

  frame(0, getColor());
}

function updatePoints(start) {
  for (let i = 0; i < nAllPts; ++i) {
    const t = 2 * Math.PI * (start + i / nAllPts);
    pts[i*4] = Math.sin(3 * t);
    pts[i*4+1] = Math.sin(2 * t);
    pts[i*4+2] = Math.sin(5 * t);
  }
}

function getColor() {
  const a = colors[clrIxFrom];
  const b = colors[clrIxTo];
  let x = clrInter;
  x = x < 0.5 ? 2 * x * x : 1 - Math.pow(-2 * x + 2, 2) / 2;
  return [
    a[0] + (b[0] - a[0]) * x,
    a[1] + (b[1] - a[1]) * x,
    a[2] + (b[2] - a[2]) * x,
  ];
}

function updateColor() {
  clrInter += 0.001;
  if (clrInter > 1) {
    clrInter = 0;
    clrIxFrom = clrIxTo;
    clrIxTo = (clrIxTo + 1) % colors.length;
  }
}

function frame(time) {

  updatePoints(time * 0.0005);
  updateColor();

  const camDist = 3;
  const camAngle = time * 0.001;
  shader.setCamPos([
    camDist * Math.sin(camAngle),
    2,
    camDist * Math.cos(camAngle),
  ]);
  shader.frame(time, getColor());
  if (animating) requestAnimationFrame(frame);
}



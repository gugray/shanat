import sSweepVert from "shader/sweep-vert.glsl";
import sMainVert from "shader/main-vert.glsl";
import sMainFrag from "shader/main-frag.glsl";
import * as twgl from "twgl.js";
import * as G from "./geo.js";

let webGLCanvas, gl, w, h;
let ptArr;
let sweepArrays, sweepBufferInfo;
let pointArrays, pointBufferInfo;
let progiMain;
let viewMatrix, projMatrix;

export let hiDef = false;

export function init(elmCanvas, pts) {

  // WebGL canvas, and twgl
  webGLCanvas = elmCanvas;
  gl = webGLCanvas.getContext("webgl2");
  twgl.addExtensionsToContext(gl);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
  gl.enable(gl.DEPTH_TEST);
  gl.depthFunc(gl.LESS);

  // This is for sweeping output range for pure fragment shaders
  sweepArrays = {
    position: {numComponents: 2, data: [-1, -1, -1, 1, 1, -1, -1, 1, 1, -1, 1, 1]},
  };
  sweepBufferInfo = twgl.createBufferInfoFromArrays(gl, sweepArrays);

  // Lissajous points
  ptArr = pts;
  pointArrays = {
    position: {numComponents: 4, data: ptArr},
  };
  pointBufferInfo = twgl.createBufferInfoFromArrays(gl, pointArrays);

  resizeWorld();
  window.addEventListener("resize", () => {
    resizeWorld();
  });

  compilePrograms();
}

function resizeWorld() {

  // Resize WebGL canvas
  const elmHost = document.getElementById("shaderbox");
  let elmWidth = elmHost.clientWidth;
  let elmHeight = elmHost.clientHeight;
  webGLCanvas.style.width = elmWidth + "px";
  webGLCanvas.style.height = elmHeight + "px";
  const mul = hiDef ? devicePixelRatio : 1;
  w = webGLCanvas.width = Math.round(elmWidth * mul);
  h = webGLCanvas.height = Math.round(elmHeight * mul);
}


function compilePrograms() {

  const del = pi => { if (pi && pi.program) gl.deleteProgram(pi.program); }
  const recreate = (v, f) => twgl.createProgramInfo(gl, [v, f]);

  // Main fragment
  const npMain = recreate(sMainVert, sMainFrag);

  if (!npMain) return false;

  del(progiMain);
  progiMain = npMain;
  return true;
}

export function setCamPos(camPosition) {
  updateProjection(camPosition, [0, 0, 0], [0, 1, 0], 45, w / h, 0.1, 100);
}

export function updateProjection(camPosition, camLookAt, camUp,
                                 viewportAngle, aspect, near, far) {
  viewMatrix = lookAt(camPosition, camLookAt, camUp);
  projMatrix = perspective(viewportAngle, aspect, near, far);
}

function lookAt(eye, target, up) {
  const zAxis = G.normalize(G.subtract(eye, target));
  const xAxis = G.normalize(G.cross(up, zAxis));
  const yAxis = G.cross(zAxis, xAxis);

  return [
    xAxis[0], yAxis[0], zAxis[0], 0,
    xAxis[1], yAxis[1], zAxis[1], 0,
    xAxis[2], yAxis[2], zAxis[2], 0,
    -G.dot(xAxis, eye), -G.dot(yAxis, eye), -G.dot(zAxis, eye), 1
  ];
}

function perspective(fov, aspect, near, far) {
  // const f = Math.tan(Math.PI * 0.5 - 0.5 * fov);
  const fovRadians = fov * Math.PI / 180;
  const f = 1 / Math.tan(fovRadians * 0.5);
  const rangeInv = 1.0 / (near - far);

  return [
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (near + far) * rangeInv, -1,
    0, 0, near * far * rangeInv * 2, 0
  ];
}

export function frame(time, clr) {

  const unisMain = {
    resolution: [w, h],
    clr: clr,
    time: time,
    view: viewMatrix,
    proj: projMatrix,
  }

  // Update point positions
  twgl.setAttribInfoBufferFromArray(gl, pointBufferInfo.attribs.position, ptArr);

  // Set up size, program, uniforms
  gl.viewport(0, 0, w, h);
  gl.useProgram(progiMain.program);
  twgl.setBuffersAndAttributes(gl, progiMain, pointBufferInfo);
  twgl.setUniforms(progiMain, unisMain);
  // Clear to black
  gl.clearColor(0, 0, 0, 1);
  gl.clear(gl.COLOR_BUFFER_BIT| gl.DEPTH_BUFFER_BIT);
  // Render fragment sweep
  twgl.drawBufferInfo(gl, pointBufferInfo, gl.POINTS);
}


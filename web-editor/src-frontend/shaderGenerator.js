import Hydra from "hydra-synth";

const canvas = document.createElement('canvas');
canvas.width = 1;
canvas.height = 1;

const hydra = new Hydra({
  detectAudio: false,
  makeGlobal: false,
  canvas: canvas,
});

export function generateShader(code) {

  // Remove comments, and trim
  code = code.replace(/\/\/.*$/gm, "").replace(/\/\*[\s\S]*?\*\//g, "");
  code = code.trim();

  const h = hydra.synth;
  try {
    const evalHydraCode = new Function('h', `with(h) { return ${code}; }`);
    const glsl = evalHydraCode(h).glsl()[0].frag;
    return [glsl, null];
  }
  catch (err) {
    if (err.message) return ["", err.message];
    else return ["", "Failed to compile code"];
  }
}

function test() {

  const h = hydra.synth;

  // const glsl = h.osc(7, 0.01, 0.8)
  //   .color(1.2,1.9,1.3)
  //   .saturate(0.4)
  //   .rotate(4, 0.8,0)
  //   .glsl()[0].frag;

  const evalHydraCode = new Function('h', `with(h) { return ${input.trim()}; }`);
  const glsl = evalHydraCode(h).glsl()[0].frag;

  console.log(glsl);
}

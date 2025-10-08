precision mediump float;

attribute vec4 position;
uniform mat4 proj;
uniform mat4 view;
varying float ofs;

void main() {


    vec4 mvPosition = view * vec4(position.xyz, 1.0);
    gl_Position = proj * mvPosition;
    gl_PointSize = 80.0 / pow(gl_Position.z, 1.2);
    ofs = position.w;

    // Sorta depth manipulation, for lack of gl_FragDepth in ES 2.0
    const float cutoff = 0.45; // Synch frag <> vert shaders
    if (ofs > cutoff) gl_Position.z += gl_Position.w * .03; 
}

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
}

#version 300 es

in vec4 position;
uniform mat4 proj;
uniform mat4 view;
out float ofs;

void main() {
    vec4 mvPosition = view * vec4(position.xyz, 1.0);
    gl_Position = proj * mvPosition;
    gl_PointSize = 80. / pow(gl_Position.z, 1.2);
    ofs = position[3];
}

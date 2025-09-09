#include "lissaj_shaders.h"

extern const char *lissaj_vert = R"(
attribute vec4 a_pos;
void main() {
    gl_Position = vec4(a_pos.xy, 0.0, 1.0);
}
)";


extern const char *lissaj_frag = R"(
precision mediump float;
uniform float time;
uniform vec2 resolution;
void main() {
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";


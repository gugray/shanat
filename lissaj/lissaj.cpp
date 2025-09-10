#include "lissaj.h"

#include <math.h>

const char *lissaj_vert = R"(
attribute vec4 a_pos;
void main() {
    gl_Position = vec4(a_pos.xy, 0.0, 1.0);
}
)";

const char *lissaj_frag = R"(
precision mediump float;
uniform float time;
uniform vec2 resolution;
void main() {
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

float LissajModel::pts[nAllPts * 4] = {0};

void LissajModel::updatePoints(float start)
{
    for (int i = 0; i < nAllPts; ++i)
    {
        const float t = 2 * M_PI * (start + i / nAllPts);
        pts[i * 4] = sin(3 * t);
        pts[i * 4 + 1] = sin(2 * t);
        pts[i * 4 + 2] = sin(5 * t);
        pts[i * 4 + 3] = (float)i / (float)nAllPts;
    }
}

#version 310 es
precision mediump float;

// UPDATE 0

uniform float time;
uniform vec2 resolution;
out vec4 fragColor;

#define PI 3.141592653

const float r = 0.2;
const float breadth = 0.01;

vec2 rot(vec2 v, float a) {
    return vec2(
        v.x * cos(a) - v.y * sin(a),
        v.x * sin(a) + v.y * cos(a));
}

float circle(vec2 uv, vec2 c, float r) {
    float d = length(uv-c);
    float val = step(r-breadth, d) * step(d, r+breadth) * 0.75;
    val += step(d, r-breadth);
    return val;
}

void main() {
    fragColor.rgb = vec3(0.0);
    fragColor.a = 1.0;

    vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;
    float aspect = resolution.x / resolution.y;
    uv.x *= aspect; // y is [-1, 1]; x is scaled

    vec2 c = vec2(0.0, 0.15);
    c = rot(c, time);

    fragColor.r = circle(uv, c, r + 0.05 * sin(time * 0.5));
    c = rot(c, PI * 2.0 / 3.0);
    fragColor.g = circle(uv, c, r + 0.05 * sin(time * 0.3));
    c = rot(c, PI * 2.0 / 3.0);
    fragColor.b = circle(uv, c, r + 0.05 * sin(time * 0.17));

    fragColor.rgb *= 0.7;
    fragColor.rgb = fragColor.rgb * fragColor.rgb;
}

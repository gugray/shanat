precision highp float;

uniform vec2 resolution;
uniform vec3 clr;
varying float ofs;

#define PI 3.141592653

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {

    // [0,1] => [-1,1]
    vec2 uv = gl_PointCoord * 2.0 - 1.0;

    float len = length(uv);
    if (len > 1.0) {
        discard;
    }

    gl_FragColor.a = 1.0;

    vec3 hsv = rgb2hsv(clr);
    float light = hsv[1];
    light *= clamp(1.3 - len * len, 0.0, 1.0);
    float sat = hsv[1];
    const float cutoff = 0.45;
    const float loLight = 0.1;
    const float loSat = 0.5;

    if (ofs > cutoff) {
        light = loLight;
        sat = loSat;
        //gl_FragDepth = -10.0; // may be ignored in ES 2.0
    }
    else {
        light = loLight + (1.0 - loLight) * light * ofs / cutoff;
        sat = loSat + (1.0 - loSat) * sat * ofs / cutoff;
    }

    vec3 finalHsv = vec3(hsv[0], sat, light);
    gl_FragColor.rgb = hsv2rgb(finalHsv);
}

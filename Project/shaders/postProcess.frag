#version 450
layout(set = 0, binding = 0) uniform sampler2D uColor;
layout(set = 0, binding = 1) uniform sampler2D uDepth;
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;
#define REVERSED_Z 0
const float NEAR_PLANE = 0.1;
const float FAR_PLANE  = 1000.0;
const float FOG_DENSITY   = 0.02;
const vec3  FOG_COLOR     = vec3(0.75, 0.85, 1.0);
const float CONTOUR_STEP  = 2.0;
const float CONTOUR_WIDTH = 0.015;
const float HEAT_BLEND    = 0.65;

float hash12(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

float linearizeDepthZO(float z, float n, float f) {
#if REVERSED_Z
    return (n * f) / max(1e-6, (n - z * (n - f)));
#else
    return (n * f) / max(1e-6, (f - z * (f - n)));
#endif
}

vec3 heatmap(float x){
    x = clamp(x, 0.0, 1.0);
    vec3 c1 = vec3(0.231, 0.298, 0.753);
    vec3 c2 = vec3(0.865, 0.865, 0.000);
    vec3 c3 = vec3(0.706, 0.016, 0.150);
    vec3 mid = mix(c1, c2, smoothstep(0.00, 0.70, x));
    vec3 end = mix(c2, c3, smoothstep(0.30, 1.00, x));
    return mix(mid, end, smoothstep(0.50, 0.90, x));
}

void main() {
    vec3 base = texture(uColor, vUV).rgb;
    float z = texture(uDepth, vUV).r;
    float d = linearizeDepthZO(z, NEAR_PLANE, FAR_PLANE);
    float dN = clamp((d - NEAR_PLANE) / (FAR_PLANE - NEAR_PLANE), 0.0, 1.0);
    vec3 depthColor = heatmap(dN);
    float freq = 6.28318530718 / CONTOUR_STEP;
    float linePattern = abs(sin(d * freq));
    float lines = 1.0 - smoothstep(1.0 - CONTOUR_WIDTH, 1.0, linePattern);
    lines *= 1.0 - smoothstep(0.0, 1.0, dN);
    float fog = 1.0 - exp(-FOG_DENSITY * d);
    fog += (hash12(gl_FragCoord.xy) - 0.5) / 255.0;
    fog = clamp(fog, 0.0, 1.0);
    vec3 fogged = mix(base, FOG_COLOR, fog);
    vec3 col = mix(fogged, depthColor, HEAT_BLEND);
    col = mix(col, vec3(1.0), lines);
    outColor = vec4(vec3(clamp((linearizeDepthZO(z, NEAR_PLANE, FAR_PLANE) - NEAR_PLANE) 
                           / (FAR_PLANE - NEAR_PLANE), 0.0, 1.0)), 1.0);
}

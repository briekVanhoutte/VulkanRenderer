#version 450
layout(set = 0, binding = 0) uniform sampler2D uColor;
layout(set = 0, binding = 1) uniform sampler2D uDepth;
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

#define REVERSED_Z 0
const float NEAR_PLANE = 0.1;
const float FAR_PLANE  = 1000.0;
const float FOG_DENSITY = 0.001;
const vec3  FOG_NEAR = vec3(1.00, 0.86, 0.93);
const vec3  FOG_FAR  = vec3(0.95, 0.35, 0.62);

float linearizeDepthZO(float z, float n, float f) {
#if REVERSED_Z
    return (n * f) / max(1e-6, (n - z * (n - f)));
#else
    return (n * f) / max(1e-6, (f - z * (f - n)));
#endif
}

void main() {
    vec3 base = texture(uColor, vUV).rgb;
    float z = texture(uDepth, vUV).r;
    float d = linearizeDepthZO(z, NEAR_PLANE, FAR_PLANE);
    float fog = clamp(1.0 - exp(-FOG_DENSITY * d), 0.0, 1.0);
    vec3 fogColor = mix(FOG_NEAR, FOG_FAR, fog);
    vec3 col = mix(base, fogColor, fog);
    outColor = vec4(col, 1.0);
}

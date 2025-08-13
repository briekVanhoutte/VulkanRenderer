#version 450

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vWorldPos;

layout(location = 0) out vec4 outColor;

// Returns a geometric (flat) normal per triangle using screen-space derivatives
void main() {
    vec3 dpdx = dFdx(vWorldPos);
    vec3 dpdy = dFdy(vWorldPos);
    vec3 N    = normalize(cross(dpdx, dpdy));

    // Make the normal consistently point "out" for backfaces
    if (!gl_FrontFacing) N = -N;

    // Visualize the face normal (0..1)
    outColor = vec4(N * 0.5 + 0.5, 1.0);
}
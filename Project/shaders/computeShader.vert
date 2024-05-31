#version 450

layout(set=0, binding=0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
} vp;

// Particle position and inverse mass
layout(location = 0) in vec4 inPositionInvMass;

void main() {
    vec3 position = inPositionInvMass.xyz;
    gl_Position = vp.proj * vp.view * vec4(position, 1.0);

    // Emit a point primitive
    gl_PointSize = 5.0; // Adjust point size as needed
}
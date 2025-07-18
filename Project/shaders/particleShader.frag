#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) in float height; // receive the height from the vertex shader

void main() {
    // Normalize height for coloring, assuming height ranges from 0 to 1
    float normalizedHeight = clamp(height, 0.1, 1.0);
    // Adjust blue color intensity based on height
    outColor = vec4(0.0, 0.0, normalizedHeight, 1.0); // blue color varies with height
}

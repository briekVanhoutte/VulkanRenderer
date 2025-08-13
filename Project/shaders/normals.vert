#version 450

layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
    vec3 cameraPos;        // std140: 16-byte slot on CPU
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint AlbedoID;
    uint NormalMapID;
    uint MetalnessID;
} mesh;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;   // unused here but kept for interface compatibility
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vWorldPos;

void main() {
    vec4 worldPos4 = mesh.model * vec4(inPosition, 1.0);
    gl_Position    = ubo.proj * ubo.view * worldPos4;

    vWorldPos = worldPos4.xyz;
    vColor    = inColor;
    vUV       = inTexCoord;
}

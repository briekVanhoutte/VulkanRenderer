#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint AlbedoID;
    uint NormalMapID;
    uint MetalnessID;
} mesh;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * mesh.model * vec4(inPosition, 1.0);
    fragNormal = normalize((mesh.model * vec4(inNormal, 0.0)).xyz);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

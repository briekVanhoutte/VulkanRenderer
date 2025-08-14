#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 proj;
    mat4 view;
    vec3 cameraPos;
} ubo;

// Per-vertex (binding = 0, locations 0..3)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

// Per-instance (binding = 1)
// Matches your InstanceData helper: mat4 rows at 4..7, uvec4 at 8, uint at 9
layout(location = 4) in mat4 inModel;
layout(location = 8) in uvec4 inTexIds0;
layout(location = 9) in uint  inHeightId;

// Varyings
layout(location = 0) out vec3 vWorldNormal;
layout(location = 1) out vec3 vColor;
layout(location = 2) out vec2 vUV;
layout(location = 3) flat out uvec4 vTexIds0;
layout(location = 4) flat out uint  vHeightId;

void main() {
    mat4 M = inModel;

    vec4 worldPos = M * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    mat3 normalMat = transpose(inverse(mat3(M)));
    vWorldNormal = normalize(normalMat * inNormal);

    vColor   = inColor;
    vUV      = inTexCoord;
    vTexIds0 = inTexIds0;
    vHeightId = inHeightId;
}

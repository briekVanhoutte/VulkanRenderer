#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler; // Texture

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    // Multiply texture with vertex color, or just use texColor if you want
    outColor = texColor * vec4(fragColor, 1.0);
}
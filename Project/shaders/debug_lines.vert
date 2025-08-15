// shaders/debug_lines.vert
#version 450
layout(location=0) in vec3 inPos;
layout(location=1) in vec4 inColor;  // from UNORM8

layout(std140, set=0, binding=0) uniform UBO {
    mat4 proj;
    mat4 view;
    vec4 cameraPos; // or vec3 with padding; vec4 is simplest
} ubo;

layout(location=0) out vec4 vColor;

layout(push_constant) uniform PC {
    mat4 world;
    float lineWidth; // not used by the shader; set via vkCmdSetLineWidth on CPU
} pc;



void main(){
    vec4 wpos = pc.world * vec4(inPos, 1.0);
    gl_Position = ubo.proj * ubo.view * wpos;
    vColor = inColor;
}

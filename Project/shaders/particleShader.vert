#version 450

layout(set=0,binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view; 
} vp;

layout(push_constant) uniform PushConstants {
    mat4 model; 
} mesh;

layout(location = 0) in vec4 inPosition; // xyz is position, w is inverted mass
layout(location = 1) out float height; // pass the height to the fragment shader

void main() {
     gl_Position = vp.proj * vp.view *  mesh.model * vec4(inPosition.xyz, 1.0);
     height = inPosition.y/2; // pass the y-coordinate as height
     gl_PointSize = 2.0;
}
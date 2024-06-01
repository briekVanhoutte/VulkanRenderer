#version 450

layout(set=0,binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view; 
} vp;

layout(location = 0) in vec4 inPosition; // xyz is position, w is inverted mass

void main() {
    gl_Position = vp.proj * vp.view * vec4(inPosition.xyz, 1.0);
     gl_PointSize = 1.0;
}
#version 450
layout(location = 0) out vec2 vUV;
void main() {
    const vec2 verts[3] = vec2[](
        vec2(-1.0, -3.0),
        vec2( 3.0,  1.0),
        vec2(-1.0,  1.0)
    );
    gl_Position = vec4(verts[gl_VertexIndex], 0.0, 1.0);
    vUV = 0.5 * (gl_Position.xy + vec2(1.0));
}
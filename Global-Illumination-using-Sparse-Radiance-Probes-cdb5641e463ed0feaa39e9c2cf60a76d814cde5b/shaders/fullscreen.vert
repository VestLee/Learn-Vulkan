#version 460

layout (location = 0) in vec2 vPosition;

layout (location = 0) out vec2 outUV;

void main(void)
{
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
}
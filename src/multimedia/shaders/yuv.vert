#version 440

layout(location = 0) in vec4 qt_VertexPosition;
layout(location = 1) in vec2 qt_VertexTexCoord;

layout(location = 0) out vec2 texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main() {
    texCoord = qt_VertexTexCoord;
    gl_Position = ubuf.matrix * qt_VertexPosition;
}

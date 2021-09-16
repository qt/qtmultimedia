#version 440

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;

layout(location = 0) out vec2 texCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float width;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main() {
    texCoord = (ubuf.colorMatrix * vec4(vertexTexCoord, 0.0, 1.0)).xy;
    gl_Position = ubuf.matrix * vertexPosition;
}

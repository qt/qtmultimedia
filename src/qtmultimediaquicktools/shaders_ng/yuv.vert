#version 440

layout(location = 0) in vec4 qt_VertexPosition;
layout(location = 1) in vec2 qt_VertexTexCoord;

layout(location = 0) out vec2 plane1TexCoord;
layout(location = 1) out vec2 plane2TexCoord;
layout(location = 2) out vec2 plane3TexCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float plane1Width;
    float plane2Width;
    float plane3Width;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main() {
    plane1TexCoord = qt_VertexTexCoord * vec2(ubuf.plane1Width, 1);
    plane2TexCoord = qt_VertexTexCoord * vec2(ubuf.plane2Width, 1);
    plane3TexCoord = qt_VertexTexCoord * vec2(ubuf.plane3Width, 1);
    gl_Position = ubuf.matrix * qt_VertexPosition;
}

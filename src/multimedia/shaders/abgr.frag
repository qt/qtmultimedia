#version 440

layout(location = 0) in vec2 qt_TexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
} ubuf;

layout(binding = 1) uniform sampler2D rgbTexture;

void main()
{
    fragColor = texture(rgbTexture, qt_TexCoord).bgra * ubuf.colorMatrix * ubuf.opacity;
}

#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float width;
} ubuf;

layout(binding = 1) uniform sampler2D rgbTexture;

void main()
{
    fragColor = ubuf.colorMatrix * texture(rgbTexture, texCoord).abgr * ubuf.opacity;
}

#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float width;
} ubuf;

layout(binding = 1) uniform sampler2D plane1Texture;

void main()
{
    vec3 YUV = texture(plane1Texture, texCoord).gba;
    float A = texture(plane1Texture, texCoord).r;
    fragColor = ubuf.colorMatrix * vec4(YUV, 1.0) * A * ubuf.opacity;
}

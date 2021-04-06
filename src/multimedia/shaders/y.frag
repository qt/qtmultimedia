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
    float Y = texture(plane1Texture, texCoord).r;
    vec4 color = vec4(Y, Y, Y, 1.);
    fragColor = ubuf.colorMatrix * color * ubuf.opacity;
}

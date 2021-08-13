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
    int x = int(floor(texCoord.x * ubuf.width));
    bool rightSubPixel = (x/2*2 != x);
    float Y = rightSubPixel ? texture(plane1Texture, texCoord).b : texture(plane1Texture, texCoord).r;
    vec2 UV = texture(plane1Texture, texCoord).ga;
    fragColor = ubuf.colorMatrix * vec4(Y, UV, 1.0) * ubuf.opacity;
}

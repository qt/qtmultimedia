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
layout(binding = 2) uniform sampler2D plane2Texture;

void main()
{
    float Y = texture(plane1Texture, texCoord).r;
    vec2 UV = texture(plane2Texture, texCoord).gr;
    vec4 color = vec4(Y, UV.x, UV.y, 1.);
    fragColor = ubuf.colorMatrix * color * ubuf.opacity;
}

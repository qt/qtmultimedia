#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
} ubuf;

layout(binding = 1) uniform sampler2D plane1Texture;
layout(binding = 2) uniform sampler2D plane2Texture;

void main()
{
    vec3 YUV = vec3(texture(plane1Texture, texCoord).g, texture(plane2Texture, texCoord).rb);
    fragColor = ubuf.colorMatrix * vec4(YUV, 1.0) * ubuf.opacity;
}

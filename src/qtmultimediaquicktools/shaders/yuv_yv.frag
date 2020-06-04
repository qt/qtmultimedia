#version 440

layout(location = 0) in vec2 plane1TexCoord;
layout(location = 1) in vec2 plane2TexCoord;
layout(location = 2) in vec2 plane3TexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float plane1Width;
    float plane2Width;
    float plane3Width;
} ubuf;

layout(binding = 1) uniform sampler2D plane1Texture;
layout(binding = 2) uniform sampler2D plane2Texture;
layout(binding = 3) uniform sampler2D plane3Texture;

void main()
{
    float Y = texture(plane1Texture, plane1TexCoord).r;
    float U = texture(plane2Texture, plane2TexCoord).r;
    float V = texture(plane3Texture, plane3TexCoord).r;
    vec4 color = vec4(Y, U, V, 1.);
    fragColor = ubuf.colorMatrix * color * ubuf.opacity;
}

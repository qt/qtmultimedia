#version 440

layout(location = 0) in vec2 qt_TexCoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float opacity;
    float u_scale;
    float v_scale;
} ubuf;

layout(binding = 1) uniform sampler2D rgbTexture;

void main()
{
    fragColor = vec4(texture(rgbTexture, qt_TexCoord * vec2(ubuf.u_scale, ubuf.v_scale)).rgb, 1.0) * ubuf.opacity;
}

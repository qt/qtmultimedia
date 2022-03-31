#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D rgbTexture;

void main()
{
    fragColor = ubuf.colorMatrix * texture(rgbTexture, texCoord).rgba * ubuf.opacity;
}

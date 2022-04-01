#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"
#include "colortransfer.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D plane1Texture;
layout(binding = 2) uniform sampler2D plane2Texture;

void main()
{
    float Y = texture(plane1Texture, texCoord).r;
    vec2 UV = texture(plane2Texture, texCoord).rg;
    vec4 color = vec4(Y, UV.x, UV.y, 1.);
    fragColor = ubuf.colorMatrix * color * ubuf.opacity;

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = convertSRGBToLinear(fragColor);
#endif
}

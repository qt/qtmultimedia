#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"
#include "colortransfer.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D plane1Texture;

void main()
{
    vec3 YUV = texture(plane1Texture, texCoord).gba;
    float A = texture(plane1Texture, texCoord).r;
    fragColor = ubuf.colorMatrix * vec4(YUV, 1.0) * A * ubuf.opacity;

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = convertSRGBToLinear(fragColor);
#endif
}

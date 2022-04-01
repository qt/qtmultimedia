#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"
#include "colortransfer.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D plane1Texture;

void main()
{
    int x = int(floor(texCoord.x * ubuf.width));
    bool rightSubPixel = (x/2*2 != x);
    float Y = rightSubPixel ? texture(plane1Texture, texCoord).a : texture(plane1Texture, texCoord).g;
    vec2 UV = texture(plane1Texture, texCoord).rb;
    fragColor = ubuf.colorMatrix * vec4(Y, UV, 1.0) * ubuf.opacity;

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = convertSRGBToLinear(fragColor);
#endif
}

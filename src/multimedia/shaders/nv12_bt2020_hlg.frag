#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"
#include "colortransfer.glsl"
#include "colorconvert.glsl"
#include "hdrtonemapper.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D plane1Texture;
layout(binding = 2) uniform sampler2D plane2Texture;

// This implements support HDR video using the HLG transfer functions, see also
//   https://en.wikipedia.org/wiki/Hybrid_log–gamma
//   https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BT.2390-6-2019-PDF-E.pdf
//   https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2100-2-201807-I!!PDF-E.pdf
//
// Tonemapping is done using the same algorithm as for the PQ transfer function, but we
// operate in HLG space here.
void main()
{
    float Y = texture(plane1Texture, texCoord).r;
    vec2 UV = texture(plane2Texture, texCoord).rg;
    // map to Rec.2020 color space
    fragColor = vec4(Y, UV.x, UV.y, 1.);
    fragColor = ubuf.colorMatrix * fragColor;

    // tonemap
    float y = (Y - 16./256.)*256./219.; // Video range (16...235)
    float scale = tonemapScaleForLuminosity(y, ubuf.masteringWhite, ubuf.maxLum);
    fragColor *= scale;

    fragColor = convertHLGToLinear(fragColor, ubuf.maxLum);
    fragColor = convertRec2020ToSRGB(fragColor);
    fragColor *= ubuf.opacity;

#ifndef QMM_OUTPUTSURFACE_LINEAR
    fragColor = convertSRGBFromLinear(fragColor);
#endif
}

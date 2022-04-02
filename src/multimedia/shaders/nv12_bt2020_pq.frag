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

// This uses the PQ transfer function, see also https://en.wikipedia.org/wiki/Perceptual_quantizer
// or https://ieeexplore.ieee.org/document/7291452
//
// Tonemapping into the RGB range supported by the output is done using
// https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BT.2390-6-2019-PDF-E.pdf, section 5.4
//
// masteringWhite in PQ values, not in linear. maxOutLum as defined in the doc above
// we assume that masteringBlack == 0, and minLum == 0 to simplify the calculations
//
// The calculation calculates a new luminosity in non linear space and scales the UV
// components before linearizing. This corresponds to option (2) at the end of section 5.4.
// This option was chosen as it keeps the colors correct while as well as being computationally
// cheapest.
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

    fragColor = convertPQToLinear(fragColor);
    fragColor = convertRec2020ToSRGB(fragColor);
    fragColor *= ubuf.opacity;

#ifndef QMM_OUTPUTSURFACE_LINEAR
    fragColor = convertSRGBFromLinear(fragColor);
#endif
}

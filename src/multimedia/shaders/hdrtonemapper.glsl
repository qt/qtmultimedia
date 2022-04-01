#ifndef HDR_TONEMAPPER
#define HDR_TONEMAPPER

#include "colortransfer.glsl"

// See https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BT.2390-6-2019-PDF-E.pdf, section 5.4
//
// masteringWhite in PQ values, not in linear. maxOutLum as defined in the doc above
// we assume that masteringBlack == 0, and minLum == 0 to simplify the calculations
//
// The calculation actually operates on luminosity and tonemaps that value within
// the bounds defined by maxLum. After that we scale the rgb values to the new
// luminosity
vec4 tonemapBT2390(vec4 rgba, float masteringWhite, float maxLum)
{
    float lum = dot(rgba, vec4(0.2627, 0.6780, 0.0593, 0.));

    // tonemapping operates in PQ space
    float p = convertPQFromLinear(lum)/masteringWhite;

    float ks = 1.5*maxLum - 0.5;

    if (p < ks)
        return rgba;

    float t = (p - ks)/(1 - ks);
    float t2 = t*t;
    float t3 = t*t2;

    p = (2*t3 - 3*t2 + 1)*ks + (t3 - 2*t2 + t)*(1. - ks) + (-2*t3 + 3*t2)*maxLum;

    // get the linear new luminosity
    float newLum = convertPQToLinear(p*masteringWhite);

    rgba *= newLum/lum;
    rgba.a = 1;
    return rgba;
}

vec4 tonemapBT2390_component(vec4 rgba, float masteringWhite, float maxLum)
{
    // tonemapping operates in PQ space
    vec4 p = convertPQFromLinear(rgba)/masteringWhite;

    float ks = 1.5*maxLum - 0.5;

    bvec4 step = lessThan(p, vec4(ks, ks, ks, 1.));

    vec4 t = (p - ks)/(1 - ks);
    vec4 t2 = t*t;
    vec4 t3 = t*t2;

    p = (2*t3 - 3*t2 + 1)*ks + (t3 - 2*t2 + t)*(1. - ks) + (-2*t3 + 3*t2)*maxLum;

    // get the linear new luminosity
    vec4 newRgba = convertPQToLinear(p*masteringWhite);

    rgba = mix(newRgba, rgba, step);
    rgba.a = 1;
    return rgba;
}

#endif

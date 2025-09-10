// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef HDR_TONEMAPPER
#define HDR_TONEMAPPER

#include "colortransfer.glsl"

// See https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-BT.2390-6-2019-PDF-E.pdf, section 5.4
//
// masteringWhite in PQ values, not in linear. maxOutLum as defined in the doc above
// we assume that masteringBlack == 0, and minLum == 0 to simplify the calculations
//
// The HDR tonemapping operates in non linear space (PQ or HLG), taking the
// non linear luminosity as input and returning a factor by which to scale
// the RGB signal so it fits within the outbut devices range.
//
// We're using the same algorithm for HLG, and do the equivalent mapping in
// HLG space.
float tonemapScaleForLuminosity(float y, float masteringWhite, float maxLum)
{
    float p = y/masteringWhite;

    float ks = 1.5*maxLum - 0.5;

    if (p < ks)
        return 1.;

    float t = (p - ks)/(1 - ks);
    float t2 = t*t;
    float t3 = t*t2;

    p = (2*t3 - 3*t2 + 1)*ks + (t3 - 2*t2 + t)*(1. - ks) + (-2*t3 + 3*t2)*maxLum;

    // get the linear new luminosity
    float newY = p*masteringWhite;

    return newY/y;
}

#endif

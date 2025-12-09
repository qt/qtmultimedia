// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"
#include "colortransfer.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D plane1Texture;

void main()
{
    // YUYV input texture is half the width of output texture
    //
    //     |  x |  y |  z |  w |
    //     | Y0 | U0 | Y1 | V0 |
    //         \         \_________________
    // RGB      \                          \
    // Output    \                          \
    // |  r |  g |  b |  a |      |  r |  g |  b |  a |
    //       x is even                   x is odd

    // When converting to RGBA, we should sample lumen Y0
    // when output column is even, and Y1 when output column
    // is odd

    float colIndex = floor(texCoord.x * ubuf.width);
    float oddOutputCol = mod(colIndex, 2);

    // dxInput is the pixel width in the half-width input texture
    vec2 dxInput = 0.5 * vec2(1 /  ubuf.width, 0);

    float oddY = texture(plane1Texture, texCoord - dxInput).z;
    float evenY = texture(plane1Texture, texCoord + dxInput).x;
    float Y =  mix(evenY, oddY, oddOutputCol);

    vec2 UV = texture(plane1Texture, texCoord).yw;
    fragColor = ubuf.colorMatrix * vec4(Y, UV, 1.0) * ubuf.opacity;

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = convertSRGBToLinear(fragColor);
#endif

    // Clamp output to valid range to account for out-of-range
    // input values and numerical inaccuracies in YUV->RGB conversion
    fragColor = clamp(fragColor, 0.0, 1.0);
}

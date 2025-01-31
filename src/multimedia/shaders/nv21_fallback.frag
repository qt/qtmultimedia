// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    float Y = texture(plane1Texture, texCoord)[ubuf.redOrAlphaIndex];

    // NV12 for GLES2.0 requires interleaved chroma plane to be packed into RGBA texture
    //
    //     |  r |  g |  b |  a |
    //     | U0 | V0 | U1 | V1 | ...
    // Original:                            Input texture:
    // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
    // | y | y | y | y | y | y | y | y |    | a | a | a | a | a | a | a | a |
    // ...                                  ...
    // | v | u | v | u | v | u | v | u |    |[r   g] [b   a]|[r   g] [b   a]|
    //                                              |               |
    //                                            sample          sample

    vec2 outputUV = vec2(0., 0.); // Output UV

    // 0       1            Column index
    // |-|-|-|-|-|-|-|-|
    //     s       s        X coordinate to sample from
    //   0   1   0   1      X coordinates of UV0 and UV1 in sampled rgba value
    float sampleWidth = 4 / ubuf.width;  // Distance between input sample x-coords
    float uvWidth = sampleWidth / 2.;    // Distance between output UV x-coords

    float colIndex = floor(texCoord.x * ubuf.width * 0.25); // 0., 0., 0., 0., 1., 1., 1., 1.
    float middleSampleX = colIndex * sampleWidth + 0.5 * sampleWidth;
    vec4 vuvu = texture(plane2Texture, vec2(middleSampleX, texCoord.y)).rgba;
    vec2 UV0 = vuvu.gr;
    vec2 UV1 = vuvu.ab;

    // X-coords of output UVs
    float x0 = middleSampleX - 0.5 * uvWidth;
    float x1 = middleSampleX + 0.5 * uvWidth;

    if (texCoord.x >= x0 && texCoord.x < x1) {
        // UV is between UV0 and UV1
        // |-|-|-|-|
        //     s
        // | 0   1 |
        //    X X
        outputUV = mix(UV0, UV1, (texCoord.x - x0) / uvWidth);
    } else if (texCoord.x >= x1) {
        // UV is after UV1
        // |-|-|-|-|-|-|-|-|
        //     s       s
        // | 0   1 | 0   1 |
        //        X
        float rightSampleX = middleSampleX + sampleWidth;
        if (rightSampleX < 1.) {
            // Mix with sampled UV0 on the right
            vec4 rightSample = texture(plane2Texture, vec2(rightSampleX, texCoord.y));
            outputUV = mix(UV1, rightSample.gr, (texCoord.x - x1) / uvWidth);
        } else {
            // Use UV1
            outputUV = UV1;
        }
    } else { // texCoord.x < x0
        // UV is before UV0
        // |-|-|-|-|-|-|-|-|
        //     s       s
        // | 0   1 | 0   1 |
        //          X
        float leftSampleX = middleSampleX - sampleWidth;
        if (leftSampleX >= 0.) {
            // Mix with sampled UV1 on the left
            vec4 leftSample = texture(plane2Texture, vec2(leftSampleX, texCoord.y));
            outputUV = mix(UV0, leftSample.ab, (x0 - texCoord.x) / uvWidth);
        } else {
            // Use UV0
            outputUV = UV0;
        }
    }

    vec4 color = vec4(Y, outputUV, 1.);
    fragColor = ubuf.colorMatrix * color * ubuf.opacity;

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = convertSRGBToLinear(fragColor);
#endif

    // Clamp output to valid range to account for out-of-range
    // input values and numerical inaccuracies in YUV->RGB conversion
    fragColor = clamp(fragColor, 0.0, 1.0);
}

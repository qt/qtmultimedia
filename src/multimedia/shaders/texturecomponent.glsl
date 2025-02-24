// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TEXTURECOMPONENT
#define TEXTURECOMPONENT

#include "uniformbuffer.glsl"
#include "qrhitextureformats_p.h"

// The function implements a workaround for platforms that don't support RG8 format.
// E.g., NV12 for GLES2.0 requires an interleaved chroma plane to be packed into RGBA texture.
//
//
//     |  r |  g |  b |  a |
//     | U0 | V0 | U1 | V1 | ...
// Original:                            Input texture:
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
// | y | y | y | y | y | y | y | y |    | a | a | a | a | a | a | a | a |
// ...                                  ...
// | u | v | u | v | u | v | u | v |    |[r   g] [b   a]|[r   g] [b   a]|
//                                              |               |
// The idea is to get the subsamples "rg" and "ba", representing uv0 and uv1 for nv12,
// and interpolate them according to the given texture coordinate.

vec2 interpolate_RGBA8_to_RG8(sampler2D planeTexture, vec2 texCoord, float frameWidth)
{
    // NV12 for GLES2.0 requires interleaved chroma plane to be packed into RGBA texture
    //
    //     |  r |  g |  b |  a |
    //     | U0 | V0 | U1 | V1 | ...
    // Original:                            Input texture:
    // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
    // | y | y | y | y | y | y | y | y |    | a | a | a | a | a | a | a | a |
    // ...                                  ...
    // | u | v | u | v | u | v | u | v |    |[r   g] [b   a]|[r   g] [b   a]|
    //                                              |               |
    //

    // 0       1            Column index
    // |-|-|-|-|-|-|-|-|
    //     s       s        X coordinate to sample from
    //   0   1   0   1      X coordinates of 2 blocks, "rg" and "ba" in sampled rgba value
    float sampleWidth = 4 / frameWidth;  // Distance between input sample x-coords
    float subSampleWidth = sampleWidth / 2.; // Distance between output "rg" and "ba" x-coords, or "uv" blocks for nv12

    float colIndex = floor(texCoord.x * frameWidth * 0.25); // 0., 0., 0., 0., 1., 1., 1., 1.
    float middleSampleX = colIndex * sampleWidth + 0.5 * sampleWidth;
    vec4 rgba = texture(planeTexture, vec2(middleSampleX, texCoord.y)).rgba;

    // X-coords of output UVs
    float x0 = middleSampleX - 0.5 * subSampleWidth;
    float x1 = middleSampleX + 0.5 * subSampleWidth;

    if (texCoord.x >= x0 && texCoord.x < x1) {
        // The target is between "rg" and "ba"
        // |-|-|-|-|
        //     s
        // | 0   1 |
        //    X X
        return mix(rgba.rg, rgba.ba, (texCoord.x - x0) / subSampleWidth);
    } else if (texCoord.x >= x1) {
        // The target is after "ba"
        // |-|-|-|-|-|-|-|-|
        //     s       s
        // | 0   1 | 0   1 |
        //        X
        float rightSampleX = middleSampleX + sampleWidth;
        if (rightSampleX < 1.) {
            // Mix with sampled "rg" on the right
            vec4 rightSample = texture(planeTexture, vec2(rightSampleX, texCoord.y));
            return mix(rgba.ba, rightSample.rg, (texCoord.x - x1) / subSampleWidth);
        } else {
            return rgba.ba;
        }
    } else { // texCoord.x < x0
        // The target is before "rg"
        // |-|-|-|-|-|-|-|-|
        //     s       s
        // | 0   1 | 0   1 |
        //          X
        float leftSampleX = middleSampleX - sampleWidth;
        if (leftSampleX >= 0.) {
            // Mix with sampled UV1 on the left
            vec4 leftSample = texture(planeTexture, vec2(leftSampleX, texCoord.y));
            return mix(rgba.rg, leftSample.ba, (x0 - texCoord.x) / subSampleWidth);
        } else {
            return rgba.rg;
        }
    }
}

float interpolate_RG8_to_R16(sampler2D planeTexture, vec2 texCoord)
{
    vec2 rg = texture(planeTexture, texCoord).rg;
    return (rg.r * 255 + rg.g * 255 * 256) / 65535;
}

vec2 interpolate_RGBA8_to_RG16(sampler2D planeTexture, vec2 texCoord)
{
    vec4 rgba = texture(planeTexture, texCoord);
    float r = (rgba.r * 255 + rgba.g * 255 * 256) / 65535;
    float g = (rgba.b * 255 + rgba.a * 255 * 256) / 65535;
    return vec2(r, g);
}

vec2 getRG8(sampler2D planeTexture, vec2 texCoord, int rhiTextureFormat) {
    if (rhiTextureFormat == RhiTextureFormat_RG8)
        return texture(planeTexture, texCoord).rg;

    if (rhiTextureFormat == RhiTextureFormat_RGBA8)
        return interpolate_RGBA8_to_RG8(planeTexture, texCoord, ubuf.width);

    return vec2(0, 0);
}

// it assumes that the rhi texture format is R8 or RED_OR_ALPHA
float getR8(sampler2D planeTexture, vec2 texCoord) {
    return texture(planeTexture, texCoord)[ubuf.redOrAlphaIndex];
}

float getR16(sampler2D planeTexture, vec2 texCoord, int rhiTextureFormat) {
    if (rhiTextureFormat == RhiTextureFormat_R16)
        return texture(planeTexture, texCoord).r;

    if (rhiTextureFormat == RhiTextureFormat_RG8)
        return interpolate_RG8_to_R16(planeTexture, texCoord);

    return 0;
}

vec2 getRG16(sampler2D planeTexture, vec2 texCoord, int rhiTextureFormat) {
    if (rhiTextureFormat == RhiTextureFormat_RG16)
        return texture(planeTexture, texCoord).rg;

    if (rhiTextureFormat == RhiTextureFormat_RGBA8)
        return interpolate_RGBA8_to_RG16(planeTexture, texCoord);

    return vec2(0, 0);
}

#endif

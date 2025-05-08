// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TEXTURECOMPONENT
#define TEXTURECOMPONENT

#include "uniformbuffer.glsl"
#include "qrhitextureformats_p.h"

float convert_RG8_to_R16(vec2 rg)
{
    return (rg.r * 255 + rg.g * 255 * 256) / 65535;
}

vec2 convert_RGBA8_to_RG16(vec4 rgba)
{
    float r = convert_RG8_to_R16(rgba.rg);
    float g = convert_RG8_to_R16(rgba.ba);
    return vec2(r, g);
}

vec2 get_RG8_from_packed_RGBA8_texture(sampler2D planeTexture, vec2 texCoord, float frameWidth)
{
    // NV12 for GLES2.0 requires interleaved chroma plane to be packed into RGBA texture
    //
    //     |  r |  g |  b |  a |
    //     | U0 | V0 | U1 | V1 | ...
    // Original (scaling factor 2):         Input texture (scaling factor 4):
    // | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
    // | y | y | y | y | y | y | y | y |    | a | a | a | a | a | a | a | a |
    // ...                                  ...
    // |[u   v]|[u   v]|[u   v]|[u   v]|    |[r   g] [b   a]|[r   g] [b   a]|
    //                                              |               |
    //

    // 0       1            Column index
    // |-|-|-|-|-|-|-|-|
    //     s       s        X coordinate to sample from
    //   0   1   0   1      X coordinates of 2 blocks, "rg" and "ba" in sampled rgba value
    float sampleWidth = 4 / frameWidth;  // Distance between input sample x-coords (scaling factor 4)
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

float get_R16_from_packed_RGBA8_texture(sampler2D planeTexture, vec2 texCoord, float frameWidth, int rhiScale)
{
    // 16-bit formats for GLES2.0 require plane to be packed into RGBA texture
    //
    // Input texture (scaling by 2):
    // |   0   |   1   |   2   |   3   |
    // | r   g   b   a | r   g   b   a |
    // |[  Y       Y  ]|[  Y       Y  ]|
    //

    float sampleWidth = rhiScale / frameWidth;  // Distance between input sample x-coords
    float subSampleWidth = sampleWidth / 2.; // Distance between output "rg" and "ba" x-coords, or "uv" blocks for nv12

    float colIndex = floor(texCoord.x / sampleWidth); // 0., 0., 1., 1., 2., 2., 3., 3.
    float middleSampleX = colIndex * sampleWidth + 0.5 * sampleWidth;
    vec4 rgba = texture(planeTexture, vec2(middleSampleX, texCoord.y)).rgba;

    // Get raw (not interpolated) float values
    float r0 = convert_RG8_to_R16(rgba.rg);
    float r1 = convert_RG8_to_R16(rgba.ba);

    // X-coords of output UVs
    float x0 = middleSampleX - 0.5 * subSampleWidth;
    float x1 = middleSampleX + 0.5 * subSampleWidth;

    if (texCoord.x >= x0 && texCoord.x < x1) {
        // The target is between "x0" and "x1"
        // |-|-|-|-|
        //     s
        // | 0   1 |
        //    X X
        return mix(r0, r1, (texCoord.x - x0) / subSampleWidth);
    } else if (texCoord.x >= x1) {
        // The target is after "x1"
        // |-|-|-|-|-|-|-|-|
        //     s       s
        // | 0   1 | 0   1 |
        //        X
        float rightSampleX = middleSampleX + sampleWidth;
        if (rightSampleX < 1.) {
            // Mix with sampled "x0" on the right
            vec4 rightSample = texture(planeTexture, vec2(rightSampleX, texCoord.y));
            return mix(r1, convert_RG8_to_R16(rightSample.rg), (texCoord.x - x1) / subSampleWidth);
        } else {
            return r1;
        }
    } else { // texCoord.x < x0
        // The target is before "x0"
        // |-|-|-|-|-|-|-|-|
        //     s       s
        // | 0   1 | 0   1 |
        //          X
        float leftSampleX = middleSampleX - sampleWidth;
        if (leftSampleX >= 0.) {
            // Mix with sampled "x1" on the left
            vec4 leftSample = texture(planeTexture, vec2(leftSampleX, texCoord.y));
            return mix(r0, convert_RG8_to_R16(leftSample.ba), (x0 - texCoord.x) / subSampleWidth);
        } else {
            return r0;
        }
    }
}

vec2 getRG8(sampler2D planeTexture, vec2 texCoord, int rhiTextureFormat) {
    if (rhiTextureFormat == RhiTextureFormat_RG8)
        return texture(planeTexture, texCoord).rg;

    if (rhiTextureFormat == RhiTextureFormat_RGBA8)
        return get_RG8_from_packed_RGBA8_texture(planeTexture, texCoord, ubuf.width);

    return vec2(0, 0);
}

// it assumes that the rhi texture format is R8 or RED_OR_ALPHA
float getR8(sampler2D planeTexture, vec2 texCoord) {
#if defined(GL_ES) && __VERSION__ < 300 // webGL1 does not support non-const indexation of vec4
    if (ubuf.redOrAlphaIndex == 0)
        return texture(planeTexture, texCoord).r;

    return texture(planeTexture, texCoord).a;
#else
    return texture(planeTexture, texCoord)[ubuf.redOrAlphaIndex];
#endif
}

float getR16(sampler2D planeTexture, vec2 texCoord, int rhiTextureFormat, int xScale) {
    if (rhiTextureFormat == RhiTextureFormat_R16)
        return texture(planeTexture, texCoord).r;

    if (rhiTextureFormat == RhiTextureFormat_RG8)
        return convert_RG8_to_R16(texture(planeTexture, texCoord).rg);

    if (rhiTextureFormat == RhiTextureFormat_RGBA8)
        return get_R16_from_packed_RGBA8_texture(planeTexture, texCoord, ubuf.width, xScale * 2);

    return 0;
}

vec2 getRG16(sampler2D planeTexture, vec2 texCoord, int rhiTextureFormat) {
    if (rhiTextureFormat == RhiTextureFormat_RG16)
        return texture(planeTexture, texCoord).rg;

    if (rhiTextureFormat == RhiTextureFormat_RGBA8)
        return convert_RGBA8_to_RG16(texture(planeTexture, texCoord).rgba);

    return vec2(0, 0);
}

#endif

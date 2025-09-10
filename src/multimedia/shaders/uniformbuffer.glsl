// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Make sure to also modify externalsampler_gles.frag when modifying this

#ifndef UNIFORMBUFFER
#define UNIFORMBUFFER

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float width;
    // HDR metadata required for tonemapping
    float masteringWhite; // in PQ or HLG values
    float maxLum; // in PQ or HLG values
    int redOrAlphaIndex; // index of the RED_OR_ALPHA component
    int plane1Format; // Rhi texture format of the 1st plane
    int plane2Format; // Rhi texture format of the 2nd plane
    int plane3Format; // Rhi texture format of the 3rd plane
} ubuf;

#endif

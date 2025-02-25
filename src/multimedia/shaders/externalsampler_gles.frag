// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#extension GL_OES_EGL_image_external : require
precision highp float;
precision highp int;

struct buf
{
    mat4 matrix;
    mat4 colorMatrix;
    float opacity;
    float width;
    float masteringWhite;
    float maxLum;
    int redOrAlphaIndex;
    int plane1Format;
    int plane2Format;
    int plane3Format;
};

uniform buf ubuf;

uniform samplerExternalOES plane1Texture;

varying vec2 texCoord;

void main()
{
    gl_FragColor = texture2D(plane1Texture, texCoord).rgba * ubuf.opacity;

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = pow(fragColor, 2.2);
#endif
}

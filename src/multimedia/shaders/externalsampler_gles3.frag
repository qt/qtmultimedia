// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#version 300 es

// ES 3.00 forbids binding/location qualifiers; QRhi binds via reflection.
#extension GL_OES_EGL_image_external_essl3 : require
precision highp float;
precision highp int;

uniform samplerExternalOES plane1Texture;

in vec2 texCoord;
out vec4 fragColor;

void main()
{
    fragColor = texture(plane1Texture, texCoord).rgba;
}

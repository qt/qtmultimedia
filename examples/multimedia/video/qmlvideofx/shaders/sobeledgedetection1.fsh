/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Based on "Graphics Shaders: Theory and Practice" (http://cgeducation.org/ShadersBook/)

uniform float dividerValue;
uniform float mixLevel;
uniform float resS;
uniform float resT;

uniform sampler2D source;
uniform lowp float qt_Opacity;
varying vec2 qt_TexCoord0;

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    vec4 c = vec4(0.0);
    if (uv.x < dividerValue) {
        vec2 st = qt_TexCoord0.st;
        vec3 irgb = texture2D(source, st).rgb;
        vec2 stp0 = vec2(1.0 / resS, 0.0);
        vec2 st0p = vec2(0.0       , 1.0 / resT);
        vec2 stpp = vec2(1.0 / resS, 1.0 / resT);
        vec2 stpm = vec2(1.0 / resS, -1.0 / resT);
        const vec3 W = vec3(0.2125, 0.7154, 0.0721);
        float i00   = dot(texture2D(source, st).rgb, W);
        float im1m1 = dot(texture2D(source, st-stpp).rgb, W);
        float ip1p1 = dot(texture2D(source, st+stpp).rgb, W);
        float im1p1 = dot(texture2D(source, st-stpm).rgb, W);
        float ip1m1 = dot(texture2D(source, st+stpm).rgb, W);
        float im10  = dot(texture2D(source, st-stp0).rgb, W);
        float ip10  = dot(texture2D(source, st+stp0).rgb, W);
        float i0m1  = dot(texture2D(source, st-st0p).rgb, W);
        float i0p1  = dot(texture2D(source, st+st0p).rgb, W);
        float h = -1.0*im1p1 - 2.0*i0p1 - 1.0*ip1p1 + 1.0*im1m1 + 2.0*i0m1 + 1.0*ip1m1;
        float v = -1.0*im1m1 - 2.0*im10 - 1.0*im1p1 + 1.0*ip1m1 + 2.0*ip10 + 1.0*ip1p1;
        float mag = 1.0 - length(vec2(h, v));
        vec3 target = vec3(mag, mag, mag);
        c = vec4(target, 1.0);
    } else {
        c = texture2D(source, qt_TexCoord0);
    }
    gl_FragColor = qt_Opacity * c;
}

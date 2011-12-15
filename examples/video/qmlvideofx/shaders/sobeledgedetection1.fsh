/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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

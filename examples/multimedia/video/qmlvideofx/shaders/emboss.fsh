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

// Based on http://kodemongki.blogspot.com/2011/06/kameraku-custom-shader-effects-example.html

uniform float dividerValue;
const float step_w = 0.0015625;
const float step_h = 0.0027778;

uniform sampler2D source;
uniform lowp float qt_Opacity;
varying vec2 qt_TexCoord0;

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    vec3 t1 = texture2D(source, vec2(uv.x - step_w, uv.y - step_h)).rgb;
    vec3 t2 = texture2D(source, vec2(uv.x, uv.y - step_h)).rgb;
    vec3 t3 = texture2D(source, vec2(uv.x + step_w, uv.y - step_h)).rgb;
    vec3 t4 = texture2D(source, vec2(uv.x - step_w, uv.y)).rgb;
    vec3 t5 = texture2D(source, uv).rgb;
    vec3 t6 = texture2D(source, vec2(uv.x + step_w, uv.y)).rgb;
    vec3 t7 = texture2D(source, vec2(uv.x - step_w, uv.y + step_h)).rgb;
    vec3 t8 = texture2D(source, vec2(uv.x, uv.y + step_h)).rgb;
    vec3 t9 = texture2D(source, vec2(uv.x + step_w, uv.y + step_h)).rgb;
    vec3 rr = -4.0 * t1 - 4.0 * t2 - 4.0 * t4 + 12.0 * t5;
    float y = (rr.r + rr.g + rr.b) / 3.0;
    vec3 col = vec3(y, y, y) + 0.3;
    if (uv.x < dividerValue)
        gl_FragColor = qt_Opacity * vec4(col, 1.0);
    else
        gl_FragColor = qt_Opacity * texture2D(source, uv);
}

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

uniform float grid;
uniform float dividerValue;
uniform float step_x;
uniform float step_y;

uniform sampler2D source;
uniform lowp float qt_Opacity;
varying vec2 qt_TexCoord0;

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    float offx = floor(uv.x  / (grid * step_x));
    float offy = floor(uv.y  / (grid * step_y));
    vec3 res = texture2D(source, vec2(offx * grid * step_x , offy * grid * step_y)).rgb;
    vec2 prc = fract(uv / vec2(grid * step_x, grid * step_y));
    vec2 pw = pow(abs(prc - 0.5), vec2(2.0));
    float  rs = pow(0.45, 2.0);
    float gr = smoothstep(rs - 0.1, rs + 0.1, pw.x + pw.y);
    float y = (res.r + res.g + res.b) / 3.0;
    vec3 ra = res / y;
    float ls = 0.3;
    float lb = ceil(y / ls);
    float lf = ls * lb + 0.3;
    res = lf * res;
    vec3 col = mix(res, vec3(0.1, 0.1, 0.1), gr);
    if (uv.x < dividerValue)
        gl_FragColor = qt_Opacity * vec4(col, 1.0);
    else
        gl_FragColor = qt_Opacity * texture2D(source, uv);
}

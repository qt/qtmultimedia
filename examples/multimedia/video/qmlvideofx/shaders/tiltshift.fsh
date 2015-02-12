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

vec3 blur()
{
    vec2 uv = qt_TexCoord0.xy;
    float y = uv.y < 0.4 ? uv.y : 1.0 - uv.y;
    float dist = 8.0 - 20.0 * y;
    vec3 acc = vec3(0.0, 0.0, 0.0);
    for (float y=-2.0; y<=2.0; ++y) {
        for (float x=-2.0; x<=2.0; ++x) {
            acc += texture2D(source, vec2(uv.x + dist * x * step_w, uv.y + 0.5 * dist * y * step_h)).rgb;
        }
    }
    return acc / 25.0;
}

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    vec3 col;
    if (uv.x > dividerValue || (uv.y >= 0.4 && uv.y <= 0.6))
        col = texture2D(source, uv).rgb;
    else
        col = blur();
    gl_FragColor = qt_Opacity * vec4(col, 1.0);
}

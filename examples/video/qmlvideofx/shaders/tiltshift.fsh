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

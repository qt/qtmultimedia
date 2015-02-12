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

uniform float targetHue;
uniform float windowWidth;
uniform float dividerValue;

uniform sampler2D source;
uniform lowp float qt_Opacity;
varying vec2 qt_TexCoord0;

void rgb2hsl(vec3 rgb, out float h, out float s, float l)
{
    float maxval = max(rgb.r, max(rgb.g, rgb.b));
    float minval = min(rgb.r, min(rgb.g, rgb.b));
    float delta = maxval - minval;
    l = (minval + maxval) / 2.0;
    s = 0.0;
    if (l > 0.0 && l < 1.0)
        s = delta / (l < 0.5 ? 2.0 * l : 2.0 - 2.0 * l);
    h = 0.0;
    if (delta > 0.0)
    {
        if (rgb.r == maxval && rgb.g != maxval)
            h += (rgb.g - rgb.b ) / delta;
        if (rgb.g == maxval && rgb.b != maxval)
            h += 2.0  + (rgb.b - rgb.r) / delta;
        if (rgb.b == maxval && rgb.r != maxval)
            h += 4.0 + (rgb.r - rgb.g) / delta;
        h *= 60.0;
    }
}

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    vec3 col = texture2D(source, uv).rgb;
    float h, s, l;
    rgb2hsl(col, h, s, l);
    float h2 = (h > targetHue) ? h - 360.0 : h + 360.0;
    float y = 0.3 * col.r + 0.59 * col.g + 0.11 * col.b;
    vec3 result;
    if (uv.x > dividerValue || (abs(h - targetHue) < windowWidth) || (abs(h2 - targetHue) < windowWidth))
        result = col;
    else
        result = vec3(y, y, y);
    gl_FragColor = qt_Opacity * vec4(result, 1.0);
}

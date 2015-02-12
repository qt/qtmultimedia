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

// Based on http://blog.qt.io/blog/2011/03/22/the-convenient-power-of-qml-scene-graph/

uniform float dividerValue;
uniform float targetWidth;
uniform float targetHeight;
uniform float time;

uniform sampler2D source;
uniform lowp float qt_Opacity;
varying vec2 qt_TexCoord0;

const float PI = 3.1415926535;
const int ITER = 7;
const float RATE = 0.1;
uniform float amplitude;
uniform float n;
uniform float pixDens;

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    vec2 tc = uv;
    vec2 p = vec2(-1.0 + 2.0 * (gl_FragCoord.x - (pixDens * 14.0)) / targetWidth, -(-1.0 + 2.0 * (gl_FragCoord.y - (pixDens * 29.0)) / targetHeight));
    float diffx = 0.0;
    float diffy = 0.0;
    vec4 col;
    if (uv.x < dividerValue) {
        for (int i=0; i<ITER; ++i) {
            float theta = float(i) * PI / float(ITER);
            vec2 r = vec2(cos(theta) * p.x + sin(theta) * p.y, -1.0 * sin(theta) * p.x + cos(theta) * p.y);
            float diff = (sin(2.0 * PI * n * (r.y + time * RATE)) + 1.0) / 2.0;
            diffx += diff * sin(theta);
            diffy += diff * cos(theta);
        }
        tc = 0.5*(vec2(1.0,1.0) + p) + amplitude * vec2(diffx, diffy);
    }
    gl_FragColor = qt_Opacity * texture2D(source, tc);
}

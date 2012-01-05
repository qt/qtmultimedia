/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

// Based on http://labs.qt.nokia.com/2011/03/22/the-convenient-power-of-qml-scene-graph/

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

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    vec2 tc = uv;
    vec2 p = vec2(-1.0 + 2.0 * gl_FragCoord.x / targetWidth, -(-1.0 + 2.0 * gl_FragCoord.y / targetHeight));
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

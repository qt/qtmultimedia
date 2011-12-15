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

// Based on http://rastergrid.com/blog/downloads/frei-chen-edge-detector/

#version 130
uniform sampler2D source;
uniform float dividerValue;
uniform float weight;
mat3 G[2] = mat3[](
    mat3( 1.0, 2.0, 1.0, 0.0, 0.0, 0.0, -1.0, -2.0, -1.0 ),
    mat3( 1.0, 0.0, -1.0, 2.0, 0.0, -2.0, 1.0, 0.0, -1.0 )
);
uniform lowp float qt_Opacity;
in vec2 qt_TexCoord0;
out vec4 FragmentColor;
void main() {
    vec2 uv = qt_TexCoord0.xy;
    vec4 c = vec4(0.0);
    if (uv.x < dividerValue) {
        mat3 intensity;
        float conv[2];
        vec3 sample;
        for (int i=0; i<3; ++i) {
            for (int j=0; j<3; ++j) {
                sample = texelFetch(source, ivec2(gl_FragCoord) + ivec2(i-1, j-1), 0).rgb;
                intensity[i][j] = length(sample) * weight;
            }
        }
        for (int i=0; i<2; ++i) {
            float dp3 = dot(G[i][0], intensity[0]) + dot(G[i][1], intensity[1]) + dot(G[i][2], intensity[2]);
            conv[i] = dp3 * dp3;
        }
        c = vec4(0.5 * sqrt(conv[0]*conv[0] + conv[1]*conv[1]));
    } else {
        c = texture2D(source, qt_TexCoord0);
    }
    FragmentColor = qt_Opacity * c;
}

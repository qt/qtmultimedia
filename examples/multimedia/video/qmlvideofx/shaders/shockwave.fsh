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

// Based on http://www.geeks3d.com/20091116/shader-library-2d-shockwave-post-processing-filter-glsl/

uniform float centerX;
uniform float centerY;
uniform float dividerValue;
uniform float granularity;
uniform float time;
uniform float weight;

uniform sampler2D source;
uniform lowp float qt_Opacity;
varying vec2 qt_TexCoord0;

void main()
{
    vec2 uv = qt_TexCoord0.xy;
    vec2 tc = qt_TexCoord0;
    vec2 center = vec2(centerX, centerY);
    const vec3 shock = vec3(10.0, 1.5, 0.1);
    if (uv.x < dividerValue) {
        float distance = distance(uv, center);
        if ((distance <= (time + shock.z)) &&
            (distance >= (time - shock.z))) {
            float diff = (distance - time);
            float powDiff = 1.0 - pow(abs(diff*shock.x), shock.y*weight);
            float diffTime = diff  * powDiff;
            vec2 diffUV = normalize(uv - center);
            tc += (diffUV * diffTime);
        }
    }
    gl_FragColor = qt_Opacity * texture2D(source, tc);
}

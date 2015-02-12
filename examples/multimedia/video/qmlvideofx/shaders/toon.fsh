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

// Based on http://www.geeks3d.com/20101029/shader-library-pixelation-post-processing-effect-glsl/

uniform float dividerValue;
uniform float threshold;
uniform float resS;
uniform float resT;
uniform float magTol;
uniform float quantize;

uniform sampler2D source;
uniform lowp float qt_Opacity;
varying vec2 qt_TexCoord0;

void main()
{
    vec4 color = vec4(1.0, 0.0, 0.0, 1.1);
    vec2 uv = qt_TexCoord0.xy;
    if (uv.x < dividerValue) {
        vec2 st = qt_TexCoord0.st;
        vec3 rgb = texture2D(source, st).rgb;
        vec2 stp0 = vec2(1.0/resS,  0.0);
        vec2 st0p = vec2(0.0     ,  1.0/resT);
        vec2 stpp = vec2(1.0/resS,  1.0/resT);
        vec2 stpm = vec2(1.0/resS, -1.0/resT);
        float i00 =   dot( texture2D(source, st).rgb, vec3(0.2125,0.7154,0.0721));
        float im1m1 = dot( texture2D(source, st-stpp).rgb, vec3(0.2125,0.7154,0.0721));
        float ip1p1 = dot( texture2D(source, st+stpp).rgb, vec3(0.2125,0.7154,0.0721));
        float im1p1 = dot( texture2D(source, st-stpm).rgb, vec3(0.2125,0.7154,0.0721));
        float ip1m1 = dot( texture2D(source, st+stpm).rgb, vec3(0.2125,0.7154,0.0721));
        float im10 =  dot( texture2D(source, st-stp0).rgb, vec3(0.2125,0.7154,0.0721));
        float ip10 =  dot( texture2D(source, st+stp0).rgb, vec3(0.2125,0.7154,0.0721));
        float i0m1 =  dot( texture2D(source, st-st0p).rgb, vec3(0.2125,0.7154,0.0721));
        float i0p1 =  dot( texture2D(source, st+st0p).rgb, vec3(0.2125,0.7154,0.0721));
        float h = -1.*im1p1 - 2.*i0p1 - 1.*ip1p1  +  1.*im1m1 + 2.*i0m1 + 1.*ip1m1;
        float v = -1.*im1m1 - 2.*im10 - 1.*im1p1  +  1.*ip1m1 + 2.*ip10 + 1.*ip1p1;
        float mag = sqrt(h*h + v*v);
        if (mag > magTol) {
            color = vec4(0.0, 0.0, 0.0, 1.0);
        }
        else {
            rgb.rgb *= quantize;
            rgb.rgb += vec3(0.5, 0.5, 0.5);
            ivec3 irgb = ivec3(rgb.rgb);
            rgb.rgb = vec3(irgb) / quantize;
            color = vec4(rgb, 1.0);
        }
    } else {
        color = texture2D(source, uv);
    }
    gl_FragColor = qt_Opacity * color;
}

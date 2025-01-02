:: Copyright (C) 2019 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.vert.qsb rgba.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.frag.qsb rgba.frag

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o yuv.vert.qsb yuv.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o yuv_yv.frag.qsb yuv_yv.frag

qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o nv12.frag.qsb nv12.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o nv21.frag.qsb nv21.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o uyvy.frag.qsb uyvy.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o yuyv.frag.qsb yuyv.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o ayuv.frag.qsb ayuv.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o p010be.frag.qsb p010be.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o p010be.frag.qsb p010be.frag

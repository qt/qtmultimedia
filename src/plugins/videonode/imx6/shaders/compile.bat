:: Copyright (C) 2020 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.vert.qsb rgba.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.frag.qsb rgba.frag

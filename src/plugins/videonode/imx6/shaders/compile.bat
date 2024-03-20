:: Copyright (C) 2020 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.vert.qsb rgba.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.frag.qsb rgba.frag

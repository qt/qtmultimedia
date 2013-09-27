/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidmultimediautils.h"

#include <qlist.h>

QT_BEGIN_NAMESPACE

int qt_findClosestValue(const QList<int> &list, int value)
{
    if (list.size() < 2)
        return 0;

    int begin = 0;
    int end = list.size() - 1;
    int pivot = begin + (end - begin) / 2;
    int v = list.at(pivot);

    while (end - begin > 1) {
        if (value == v)
            return pivot;

        if (value > v)
            begin = pivot;
        else
            end = pivot;

        pivot = begin + (end - begin) / 2;
        v = list.at(pivot);
    }

    return value - v >= list.at(pivot + 1) - value ? pivot + 1 : pivot;
}

bool qt_sizeLessThan(const QSize &s1, const QSize &s2)
{
    return s1.width() * s1.height() < s2.width() * s2.height();
}

void qt_convert_NV21_to_ARGB32(const uchar *yuv, quint32 *rgb, int width, int height)
{
    const int frameSize = width * height;

    int a = 0;
    for (int i = 0, ci = 0; i < height; ++i, ci += 1) {
        for (int j = 0, cj = 0; j < width; ++j, cj += 1) {
            int y = (0xff & ((int) yuv[ci * width + cj]));
            int v = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 0]));
            int u = (0xff & ((int) yuv[frameSize + (ci >> 1) * width + (cj & ~1) + 1]));
            y = y < 16 ? 16 : y;

            int r = (int) (1.164f * (y - 16) + 1.596f * (v - 128));
            int g = (int) (1.164f * (y - 16) - 0.813f * (v - 128) - 0.391f * (u - 128));
            int b = (int) (1.164f * (y - 16) + 2.018f * (u - 128));

            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            rgb[a++] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
    }
}

QT_END_NAMESPACE

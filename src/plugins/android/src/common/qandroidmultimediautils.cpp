/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

QVideoFrame::PixelFormat qt_pixelFormatFromAndroidImageFormat(AndroidCamera::ImageFormat f)
{
    switch (f) {
    case AndroidCamera::NV21:
        return QVideoFrame::Format_NV21;
    case AndroidCamera::YV12:
        return QVideoFrame::Format_YV12;
    case AndroidCamera::RGB565:
        return QVideoFrame::Format_RGB565;
    case AndroidCamera::YUY2:
        return QVideoFrame::Format_YUYV;
    case AndroidCamera::JPEG:
        return QVideoFrame::Format_Jpeg;
    default:
        return QVideoFrame::Format_Invalid;
    }
}

AndroidCamera::ImageFormat qt_androidImageFormatFromPixelFormat(QVideoFrame::PixelFormat f)
{
    switch (f) {
    case QVideoFrame::Format_NV21:
        return AndroidCamera::NV21;
    case QVideoFrame::Format_YV12:
        return AndroidCamera::YV12;
    case QVideoFrame::Format_RGB565:
        return AndroidCamera::RGB565;
    case QVideoFrame::Format_YUYV:
        return AndroidCamera::YUY2;
    case QVideoFrame::Format_Jpeg:
        return AndroidCamera::JPEG;
    default:
        return AndroidCamera::UnknownImageFormat;
    }
}

QT_END_NAMESPACE

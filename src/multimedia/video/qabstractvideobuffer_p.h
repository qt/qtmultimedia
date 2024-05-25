// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTVIDEOBUFFER_H
#define QABSTRACTVIDEOBUFFER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/qtvideo.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QAbstractVideoBuffer
{
public:
    struct MapData
    {
        int nPlanes = 0;
        int bytesPerLine[4] = {};
        uchar *data[4] = {};
        int size[4] = {};
    };

    virtual ~QAbstractVideoBuffer();
    virtual MapData map(QtVideo::MapMode mode) = 0;
    virtual void unmap() = 0;
    virtual QVideoFrameFormat format() const = 0;
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QtVideo::MapMode);
#endif

QT_END_NAMESPACE

#endif

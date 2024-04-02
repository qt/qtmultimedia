// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEOFRAME_P_H
#define QVIDEOFRAME_P_H

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

#include "qvideoframe.h"
#include "qabstractvideobuffer_p.h"

#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QVideoFramePrivate : public QSharedData
{
public:
    QVideoFramePrivate() = default;
    QVideoFramePrivate(const QVideoFrameFormat &format) : format(format) { }

    static QVideoFramePrivate *handle(QVideoFrame &frame) { return frame.d.get(); };

    QVideoFrame adoptThisByVideoFrame()
    {
        QVideoFrame frame;
        frame.d = QExplicitlySharedDataPointer(this, QAdoptSharedDataTag{});
        return frame;
    }

    qint64 startTime = -1;
    qint64 endTime = -1;
    QAbstractVideoBuffer::MapData mapData;
    QVideoFrameFormat format;
    std::unique_ptr<QAbstractVideoBuffer> buffer;
    int mappedCount = 0;
    QMutex mapMutex;
    QString subtitleText;
    QtVideo::Rotation rotation = QtVideo::Rotation::None;
    bool mirrored = false;
    QImage image;
    QMutex imageMutex;

private:
    Q_DISABLE_COPY(QVideoFramePrivate)
};

QT_END_NAMESPACE

#endif // QVIDEOFRAMEPRIVATE_P_H

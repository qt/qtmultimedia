// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEMORYVIDEOBUFFER_P_H
#define QMEMORYVIDEOBUFFER_P_H

#include <private/qabstractvideobuffer_p.h>
#include <qvideoframe.h>

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

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QMemoryVideoBuffer : public QAbstractVideoBuffer
{
public:
    QMemoryVideoBuffer(const QByteArray &data, int bytesPerLine);
    ~QMemoryVideoBuffer();

    QVideoFrame::MapMode mapMode() const override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    int bytesPerLine = 0;
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QByteArray data;
};

QT_END_NAMESPACE


#endif

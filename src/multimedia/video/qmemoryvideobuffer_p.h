// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEMORYVIDEOBUFFER_P_H
#define QMEMORYVIDEOBUFFER_P_H

#include <private/qabstractvideobuffer_p.h>

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
    QMemoryVideoBuffer(QByteArray data, int bytesPerLine);
    ~QMemoryVideoBuffer() override;

    MapData map(QtVideo::MapMode mode) override;
    void unmap() override;

    QVideoFrameFormat format() const override { return {}; }

private:
    int m_bytesPerLine = 0;
    QtVideo::MapMode m_mapMode = QtVideo::MapMode::NotMapped;
    QByteArray m_data;
};

QT_END_NAMESPACE


#endif

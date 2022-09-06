// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMOCKVIDEOBUFFER_H
#define QMOCKVIDEOBUFFER_H

#include "qimage.h"
#include "private/qabstractvideobuffer_p.h"

class QMockVideoBuffer : public QAbstractVideoBuffer
{
public:
    QMockVideoBuffer(QImage image) : QAbstractVideoBuffer(QVideoFrame::NoHandle), m_image(image) { }

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped && !m_image.isNull()
            && mode != QVideoFrame::NotMapped) {
            m_mapMode = mode;

            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = m_image.bytesPerLine();
            mapData.data[0] = m_image.bits();
            mapData.size[0] = m_image.sizeInBytes();
        }

        return mapData;
    }

    void unmap() override { m_mapMode = QVideoFrame::NotMapped; }

private:
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QImage m_image;
};

#endif // QMOCKVIDEOBUFFER_H

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKVIDEOBUFFER_H
#define QMOCKVIDEOBUFFER_H

#include "qimage.h"
#include "private/qhwvideobuffer_p.h"

class QMockVideoBuffer : public QHwVideoBuffer
{
public:
    QMockVideoBuffer(QImage image) : QHwVideoBuffer(QVideoFrame::NoHandle), m_image(image) { }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped && !m_image.isNull()
            && mode != QVideoFrame::NotMapped) {
            m_mapMode = mode;

            mapData.planeCount = 1;
            mapData.bytesPerLine[0] = m_image.bytesPerLine();
            mapData.data[0] = m_image.bits();
            mapData.dataSize[0] = m_image.sizeInBytes();
        }

        return mapData;
    }

    void unmap() override { m_mapMode = QVideoFrame::NotMapped; }

private:
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QImage m_image;
};

#endif // QMOCKVIDEOBUFFER_H

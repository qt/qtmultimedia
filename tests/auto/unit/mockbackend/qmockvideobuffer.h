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

    MapData map(QtVideo::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QtVideo::MapMode::NotMapped && !m_image.isNull()
            && mode != QtVideo::MapMode::NotMapped) {
            m_mapMode = mode;

            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = m_image.bytesPerLine();
            mapData.data[0] = m_image.bits();
            mapData.size[0] = m_image.sizeInBytes();
        }

        return mapData;
    }

    void unmap() override { m_mapMode = QtVideo::MapMode::NotMapped; }

private:
    QtVideo::MapMode m_mapMode = QtVideo::MapMode::NotMapped;
    QImage m_image;
};

#endif // QMOCKVIDEOBUFFER_H

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qimagevideobuffer_p.h"

QT_BEGIN_NAMESPACE

QImageVideoBuffer::QImageVideoBuffer(QImage image)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle), m_image(std::move(image))
{
}

QVideoFrame::MapMode QImageVideoBuffer::mapMode() const
{
    return m_mapMode;
}

QAbstractVideoBuffer::MapData QImageVideoBuffer::map(QVideoFrame::MapMode mode)
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

void QImageVideoBuffer::unmap()
{
    m_mapMode = QVideoFrame::NotMapped;
}

QT_END_NAMESPACE

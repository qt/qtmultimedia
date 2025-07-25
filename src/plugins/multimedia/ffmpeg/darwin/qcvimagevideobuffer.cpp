// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qcvimagevideobuffer_p.h>

QT_BEGIN_NAMESPACE

QFFmpeg::CVImageVideoBuffer::CVImageVideoBuffer(QAVFHelpers::QSharedCVPixelBuffer &&pixelBuffer)
    : m_buffer(std::move(pixelBuffer))
{
    Q_ASSERT(m_buffer);
}

QFFmpeg::CVImageVideoBuffer::~CVImageVideoBuffer()
{
    Q_ASSERT(m_mode == QVideoFrame::NotMapped);
}

QAbstractVideoBuffer::MapData QFFmpeg::CVImageVideoBuffer::map(QVideoFrame::MapMode mode)
{
    Q_ASSERT(m_buffer);

    MapData mapData;

    if (m_mode == QVideoFrame::NotMapped) {
        CVPixelBufferLockBaseAddress(
            m_buffer.get(),
            mode == QVideoFrame::ReadOnly ? kCVPixelBufferLock_ReadOnly : 0);
        m_mode = mode;
    }

    mapData.planeCount = CVPixelBufferGetPlaneCount(m_buffer.get());
    Q_ASSERT(mapData.planeCount <= 3);

    if (!mapData.planeCount) {
        // single plane
        mapData.bytesPerLine[0] = CVPixelBufferGetBytesPerRow(m_buffer.get());
        mapData.data[0] = static_cast<uchar *>(CVPixelBufferGetBaseAddress(m_buffer.get()));
        mapData.dataSize[0] = CVPixelBufferGetDataSize(m_buffer.get());
        mapData.planeCount = mapData.data[0] ? 1 : 0;
        return mapData;
    }

    // For a bi-planar or tri-planar format we have to set the parameters correctly:
    for (int i = 0; i < mapData.planeCount; ++i) {
        mapData.bytesPerLine[i] = CVPixelBufferGetBytesPerRowOfPlane(m_buffer.get(), i);
        mapData.dataSize[i] =
            mapData.bytesPerLine[i] * CVPixelBufferGetHeightOfPlane(m_buffer.get(), i);
        mapData.data[i] =
            static_cast<uchar *>(CVPixelBufferGetBaseAddressOfPlane(m_buffer.get(), i));
    }

    return mapData;
}

void QFFmpeg::CVImageVideoBuffer::unmap()
{
    Q_ASSERT(m_buffer);
    if (m_mode != QVideoFrame::NotMapped) {
        CVPixelBufferUnlockBaseAddress(
            m_buffer.get(),
            m_mode == QVideoFrame::ReadOnly ? kCVPixelBufferLock_ReadOnly : 0);
        m_mode = QVideoFrame::NotMapped;
    }
}

QT_END_NAMESPACE

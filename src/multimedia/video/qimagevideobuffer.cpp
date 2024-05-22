// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qimagevideobuffer_p.h"
#include "qvideoframeformat.h"

QT_BEGIN_NAMESPACE

namespace {

QImage::Format fixImageFormat(QImage::Format format)
{
    switch (format) {
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_A2RGB30_Premultiplied:
    case QImage::Format_RGBA64_Premultiplied:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return QImage::Format_ARGB32_Premultiplied;
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
    case QImage::Format_Alpha8:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA32FPx4:
        return QImage::Format_ARGB32;
    case QImage::Format_Invalid:
        return QImage::Format_Invalid;
    default:
        return QImage::Format_RGB32;
    }
}

QImage fixImage(QImage image)
{
    if (image.format() == QImage::Format_Invalid)
        return image;

    const auto frameFormat = QVideoFrameFormat::pixelFormatFromImageFormat(image.format());
    if (frameFormat != QVideoFrameFormat::Format_Invalid)
        return image;

    return image.convertToFormat(fixImageFormat(image.format()));
}

} // namespace

QImageVideoBuffer::QImageVideoBuffer(QImage image)
    : QAbstractVideoBuffer(QVideoFrame::NoHandle), m_image(fixImage(std::move(image)))
{
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

QImage QImageVideoBuffer::underlyingImage() const
{
    return m_image;
}

QT_END_NAMESPACE

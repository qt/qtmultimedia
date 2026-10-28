// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdmabufsource_p.h"

#include <QtMultimedia/private/qdmabufallocator_p.h>
#include <QtMultimedia/private/qmultimedia_dmabuf_support_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <sys/mman.h>

#if QT_CONFIG(linux_dmabuf)

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {

bool dmaBufIsSemiPlanar(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
        return true;
    default:
        return false;
    }
}

} // namespace

std::optional<DmaBufLayout> dmaBufLayoutForFormat(QVideoFrameFormat::PixelFormat format,
                                                  QSize frameSize, int bytesPerLine,
                                                  quint32 sizeImage)
{
    // Compressed frames have no pitch or plane layout, even though they map to a DRM fourcc.
    if (format == QVideoFrameFormat::Format_Jpeg)
        return std::nullopt;

    const auto *desc = QVideoTextureHelper::textureDescription(format);
    const auto drmFormats = dmaBufFourccFromPixelFormat(format);
    if (!desc || drmFormats.empty() || bytesPerLine <= 0)
        return std::nullopt;

    const bool supported = drmFormats.size() == 1 || dmaBufIsSemiPlanar(format);
    if (!supported)
        return std::nullopt;

    DmaBufLayout layout;
    qsizetype offset = 0;
    for (qsizetype plane = 0; plane < drmFormats.size(); ++plane) {
        const qsizetype size =
                qsizetype(bytesPerLine) * desc->heightForPlane(frameSize.height(), int(plane));
        layout.push_back(DmaBufPlaneLayout{
                uint32_t(offset),
                uint32_t(bytesPerLine),
                size,
        });
        offset += size;
    }

    if (offset > qsizetype(sizeImage))
        return std::nullopt;

    return layout;
}

DmaBufSource::DmaBufSource() = default;

DmaBufSource::DmaBufSource(std::shared_ptr<const QDmaBuf> buffer, DmaBufLayout layout)
    : m_buffer(std::move(buffer)), m_layout(std::move(layout))
{
    Q_ASSERT(m_buffer);
    Q_ASSERT(!m_layout.isEmpty() && m_layout[0].offset == 0);
    Q_ASSERT(qsizetype(m_layout.back().offset) + m_layout.back().size <= m_buffer->size());
}

std::optional<QAbstractVideoBuffer::MapData> DmaBufSource::mapReadOnly() const
{
    Q_ASSERT(m_buffer);

    void *mapped =
            ::mmap(nullptr, size_t(m_buffer->size()), PROT_READ, MAP_SHARED, m_buffer->fd(), 0);
    if (mapped == MAP_FAILED)
        return std::nullopt;

    dmaBufSyncRead(m_buffer->fd(), DmaBufCpuAccess::Begin);

    QAbstractVideoBuffer::MapData mapData;
    mapData.planeCount = int(m_layout.size());
    for (qsizetype plane = 0; plane < m_layout.size(); ++plane) {
        const DmaBufPlaneLayout &p = m_layout[plane];
        mapData.data[plane] = static_cast<uchar *>(mapped) + p.offset;
        mapData.bytesPerLine[plane] = int(p.pitch);
        mapData.dataSize[plane] = int(p.size);
    }
    return mapData;
}

void DmaBufSource::unmap(QAbstractVideoBuffer::MapData &mapData) const
{
    Q_ASSERT(m_buffer);

    dmaBufSyncRead(m_buffer->fd(), DmaBufCpuAccess::End);
    ::munmap(mapData.data[0], size_t(m_buffer->size()));
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QT_CONFIG(linux_dmabuf)

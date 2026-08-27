// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvivantevideobuffer_p.h"

#include <QtMultimedia/private/qrhivaluemapper_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimedia/private/qvivantetextureimporter_p.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qloggingcategory.h>

static_assert(QT_CONFIG(gpu_vivante));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcVivanteVideoBuffer, "qt.multimedia.vivantevideobuffer");

namespace QtMultimediaPrivate {

static QRhiValueMapper<VivanteContext> g_vivanteContexts;
VivanteContext &vivanteContextForRhi(QRhi &rhi)
{
    return g_vivanteContexts.getOrCreate(rhi, [&] {
        return VivanteContext(&rhi);
    });
}

QVivanteVideoBuffer::QVivanteVideoBuffer(QVideoFrameFormat::PixelFormat format, QSize size,
                                         quint32 physicalAddress, void *logicalAddr,
                                         GLenum vivFormatToken, std::shared_ptr<void> keepAlive,
                                         VivanteCpuView cpuView)
    : m_format(format),
      m_size(size),
      m_physicalAddress(physicalAddress),
      m_logicalAddr(logicalAddr),
      m_vivFormatToken(vivFormatToken),
      m_cpuView(cpuView),
      m_keepAlive(std::move(keepAlive))
{
}

QVivanteVideoBuffer::~QVivanteVideoBuffer()
{
    Q_ASSERT(m_mapMode == QVideoFrame::NotMapped);
}

QVideoFrameTexturesUPtr QVivanteVideoBuffer::mapTextures(QRhi &rhi)
{
    Q_ASSERT(rhi.thread()->isCurrentThread());

    if (rhi.backend() != QRhi::OpenGLES2)
        return {};

    VivanteContext &context = vivanteContextForRhi(rhi);

    if (!context.isValid()) {
        qCDebug(qLcVivanteVideoBuffer) << "VivanteContext is not valid, falling back to CPU path";
        return {};
    }

    auto handles = importVivanteTexture(rhi, context, m_physicalAddress, m_logicalAddr, m_size,
                                        m_vivFormatToken, m_keepAlive);
    if (!handles) {
        qCWarning(qLcVivanteVideoBuffer) << "Vivante direct-texture import failed";
        return {};
    }

    return QVideoTextureHelper::createTexturesFromHandles(std::move(*handles), rhi, m_format,
                                                          m_size);
}

QAbstractVideoBuffer::MapData QVivanteVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData mapData;

    // The producer (VPU/driver) owns write access to the phys-mem buffer.
    if (mode != QVideoFrame::ReadOnly || m_mapMode != QVideoFrame::NotMapped)
        return mapData;

    if (!m_cpuView.data) {
        qCDebug(qLcVivanteVideoBuffer) << "No CPU-mapped view available for this buffer";
        return mapData;
    }

    // The caller must have already CPU-mapped the phys-mem buffer (e.g. via gst_buffer_map())
    // and passed the pointer in as VivanteCpuView; just hand that view back out.
    mapData.planeCount = 1;
    mapData.data[0] = m_cpuView.data;
    mapData.bytesPerLine[0] = m_cpuView.bytesPerLine;
    mapData.dataSize[0] = m_cpuView.dataSize;

    m_mapMode = mode;
    return mapData;
}

void QVivanteVideoBuffer::unmap()
{
    // Nothing to release: the CPU-mapped view is owned by the caller (see
    // VivanteCpuView / m_keepAlive), not by this object.
    m_mapMode = QVideoFrame::NotMapped;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

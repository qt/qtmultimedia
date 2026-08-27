// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QVIVANTEVIDEOBUFFER_P_H
#define QVIVANTEVIDEOBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qhwvideobuffer_p.h"

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtGui/qopenglfunctions.h>

#include <QtCore/qsize.h>

#include <memory>

static_assert(QT_CONFIG(gpu_vivante));

QT_BEGIN_NAMESPACE

class QRhi;

namespace QtMultimediaPrivate {

// An already CPU-mapped, flat view of a Vivante phys-mem buffer, provided
// by the caller (e.g. after gst_buffer_map() on the GStreamer side).
// QVivanteVideoBuffer has no fd to mmap itself, so map()/unmap() just hand
// this view back out.
struct VivanteCpuView
{
    uchar *data = nullptr;
    int bytesPerLine = 0;
    int dataSize = 0;
};

class Q_MULTIMEDIA_EXPORT QVivanteVideoBuffer final : public QHwVideoBuffer
{
public:
    // physicalAddress/logicalAddr describe the Vivante phys-mem allocation
    // for the zero-copy glTexDirectVIVMap path; physicalAddress is a 32-bit
    // hardware address (e.g. gst_phys_memory_get_phys_addr()), not a host
    // pointer. cpuView, if provided, backs the CPU fallback map()/unmap()
    // (see VivanteCpuView above). keepAlive pins whatever owns this memory
    // (e.g. a GstBuffer ref) alive for the buffer's lifetime -- there's no
    // fd here to dup.
    QVivanteVideoBuffer(QVideoFrameFormat::PixelFormat format, QSize, quint32 physicalAddress,
                        void *logicalAddr, GLenum vivFormatToken,
                        std::shared_ptr<void> keepAlive = {},
                        VivanteCpuView cpuView = {});
    ~QVivanteVideoBuffer() override;

    QVideoFrameTexturesUPtr mapTextures(QRhi &) override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    QVideoFrameFormat::PixelFormat pixelFormat() const { return m_format; }
    QSize size() const { return m_size; }

private:
    const QVideoFrameFormat::PixelFormat m_format;
    const QSize m_size;
    const quint32 m_physicalAddress;
    void *const m_logicalAddr;
    const GLenum m_vivFormatToken;
    const VivanteCpuView m_cpuView;

    const std::shared_ptr<void> m_keepAlive;

    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QVIVANTEVIDEOBUFFER_P_H

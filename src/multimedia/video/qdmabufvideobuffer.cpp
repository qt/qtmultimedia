// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdmabufvideobuffer_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimedia/private/qrhivaluemapper_p.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qloggingcategory.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcDmaBufVideoBuffer, "qt.multimedia.dmabufvideobuffer");

namespace QtMultimediaPrivate {

static QRhiValueMapper<DmaBufEglContext> g_eglContexts;

QDmaBufVideoBuffer::QDmaBufVideoBuffer(QVideoFrameFormat::PixelFormat format, QSize size,
                                       QSpan<const DmaBufPlane> planes,
                                       std::shared_ptr<void> keepAlive)
    : QHwVideoBuffer(QVideoFrame::NoHandle),
      m_format(format),
      m_size(size),
      m_planeCount(std::clamp(0, int(planes.size()), 4)),
      m_keepAlive(std::move(keepAlive))
{
    Q_ASSERT(!planes.empty() && planes.size() <= 4);

    for (int i = 0; i < m_planeCount; ++i) {
        m_planes[i] = planes[i];

        // Take our own, independent reference: the producer may close its
        // fd at any point while this buffer is still alive.
        if (m_planes[i].fd >= 0) {
            const int dupped = ::fcntl(m_planes[i].fd, F_DUPFD_CLOEXEC, 0);
            if (dupped < 0) {
                qCWarning(qLcDmaBufVideoBuffer)
                        << "Failed to dup DMABUF fd for plane" << i << ':' << strerror(errno);
                m_planes[i].fd = -1;
            } else {
                m_planes[i].fd = dupped;
            }
        }
    }
}

QDmaBufVideoBuffer::~QDmaBufVideoBuffer()
{
    Q_ASSERT(m_mapMode == QVideoFrame::NotMapped);

    for (int i = 0; i < m_planeCount; ++i) {
        if (m_planes[i].fd >= 0)
            ::close(m_planes[i].fd);
    }
}

QVideoFrameTexturesUPtr QDmaBufVideoBuffer::mapTextures(QRhi &rhi,
                                                        QVideoFrameTexturesUPtr &oldTextures)
{
    Q_UNUSED(oldTextures);
    Q_ASSERT(rhi.thread()->isCurrentThread());

    if (rhi.backend() != QRhi::OpenGLES2)
        return {};

    // DmaBufEglContext is bound to the GL context associated with a QRhi;
    // textures are (re-)imported on the RHI render thread, so this is
    // created lazily here (rather than in the constructor, which may run on
    // a different thread) and cached per-rhi.
    DmaBufEglContext &eglContext = g_eglContexts.getOrCreate(rhi, [&] {
        return DmaBufEglContext(&rhi);
    });

    if (!eglContext.isValid()) {
        qCDebug(qLcDmaBufVideoBuffer) << "DmaBufEglContext is not valid, falling back to CPU path";
        return {};
    }

    auto handles = importDmaBufTextures(rhi, eglContext, QSpan(m_planes.data(), m_planeCount),
                                        m_format, m_size, m_keepAlive);
    if (!handles) {
        if (handles.error() == FailureSeverity::unrecoverable) {
            qCWarning(qLcDmaBufVideoBuffer)
                    << "Unrecoverable EGL import failure, disabling texture import for this buffer";
            eglContext.invalidate();
        }
        return {};
    }

    return QVideoTextureHelper::createTexturesFromHandles(std::move(*handles), rhi, m_format,
                                                          m_size);
}

QAbstractVideoBuffer::MapData QDmaBufVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData mapData;

    // The producer (driver/compositor) owns write access to the DMABUF.
    if (mode != QVideoFrame::ReadOnly || m_mapMode != QVideoFrame::NotMapped)
        return mapData;

    auto *desc = QVideoTextureHelper::textureDescription(m_format);
    if (!desc)
        return mapData;

    QVarLengthArray<std::pair<int, std::pair<void *, size_t>>, 4> mappedByFd;

    for (int i = 0; i < m_planeCount; ++i) {
        const DmaBufPlane &plane = m_planes[i];

        void *base = nullptr;
        for (const auto &entry : mappedByFd) {
            if (entry.first == plane.fd) {
                base = entry.second.first;
                break;
            }
        }

        if (!base) {
            // Determine the size of the DMABUF via lseek (dma-buf file
            // objects implement llseek to report their size), then mmap it
            // read-only in one go; individual planes just index into this
            // mapping via their offset.
            const off_t len = ::lseek(plane.fd, 0, SEEK_END);
            if (len <= 0) {
                qCWarning(qLcDmaBufVideoBuffer) << "Could not determine DMABUF size for plane" << i;
                for (const auto &entry : mappedByFd)
                    ::munmap(entry.second.first, entry.second.second);
                return {};
            }

            void *mapped = ::mmap(nullptr, size_t(len), PROT_READ, MAP_SHARED, plane.fd, 0);
            if (mapped == MAP_FAILED) {
                qCWarning(qLcDmaBufVideoBuffer)
                        << "mmap failed for plane" << i << ':' << strerror(errno);
                for (const auto &entry : mappedByFd)
                    ::munmap(entry.second.first, entry.second.second);
                return {};
            }

            mappedByFd.push_back({ plane.fd, { mapped, size_t(len) } });
            base = mapped;
        }

        mapData.data[i] = static_cast<uchar *>(base) + plane.offset;
        mapData.bytesPerLine[i] = int(plane.pitch);
        mapData.dataSize[i] = int(plane.pitch) * desc->heightForPlane(m_size.height(), i);
    }

    mapData.planeCount = m_planeCount;

    m_mappedRegions.clear();
    for (const auto &entry : mappedByFd)
        m_mappedRegions.push_back(entry.second);

    m_mapMode = mode;
    return mapData;
}

void QDmaBufVideoBuffer::unmap()
{
    for (const auto &region : m_mappedRegions)
        ::munmap(region.first, region.second);
    m_mappedRegions.clear();

    m_mapMode = QVideoFrame::NotMapped;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

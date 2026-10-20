// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdmabufvideobuffer_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimedia/private/qrhivaluemapper_p.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qcore_unix_p.h>

#if QT_CONFIG(opengl) && QT_CONFIG(egl)
#  include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#endif

#include <cerrno>
#include <cstring>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcDmaBufVideoBuffer, "qt.multimedia.dmabufvideobuffer");

namespace QtMultimediaPrivate {

#if QT_CONFIG(opengl) && QT_CONFIG(egl)
static QRhiValueMapper<DmaBufEglContext> g_eglContexts;
#endif

namespace {

q23::expected<QUniqueFileDescriptorHandle, std::error_code> dupFd(int fd)
{
    QUniqueFileDescriptorHandle duped{ ::fcntl(fd, F_DUPFD_CLOEXEC, 0) };
    if (!duped)
        return q23::unexpected(std::error_code(errno, std::generic_category()));
    return duped;
}

q23::expected<DmaBufPlane, std::error_code> planeWithDupedFd(DmaBufPlane plane)
{
    auto duped = dupFd(plane.fd);
    if (!duped)
        return q23::unexpected(duped.error());

    plane.fd = duped->release();
    return plane;
}

} // namespace

QDmaBufVideoBuffer::QDmaBufVideoBuffer(QVideoFrameFormat::PixelFormat format, QSize size,
                                       QSpan<const DmaBufPlane> planes,
                                       std::shared_ptr<void> keepAlive, DmaBufImportMode importMode)
    : m_format(format), m_size(size), m_keepAlive(std::move(keepAlive)), m_importMode(importMode)
{
    Q_ASSERT(!planes.empty() && planes.size() <= 4);

    for (qsizetype i = 0; i < planes.size(); ++i) {
        q23::expected<DmaBufPlane, std::error_code> plane = planeWithDupedFd(planes[i]);
        if (!plane) {
            qCWarning(qLcDmaBufVideoBuffer)
                    << "Failed to dup DMABUF fd for plane" << i << ':' << plane.error().message();
            break;
        }

        m_planes.push_back(std::move(*plane));
    }
}

QDmaBufVideoBuffer::~QDmaBufVideoBuffer()
{
    Q_ASSERT(m_mapMode == QVideoFrame::NotMapped);

    for (DmaBufPlane &plane : m_planes) {
        if (plane.fd >= 0)
            qt_safe_close(plane.fd);
    }
}

QVideoFrameTexturesUPtr QDmaBufVideoBuffer::mapTextures(QRhi &rhi)
{
    Q_ASSERT(rhi.thread()->isCurrentThread());

    if (rhi.backend() != QRhi::OpenGLES2)
        return {};

#if QT_CONFIG(opengl) && QT_CONFIG(egl)
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

    auto handles = importDmaBufTextures(rhi, eglContext, m_planes, m_format, m_size, m_keepAlive,
                                        m_importMode);
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
#else // QT_CONFIG(opengl) && QT_CONFIG(egl)
    qCDebug(qLcDmaBufVideoBuffer)
            << "DMABUF texture import is not supported in this build, falling back to CPU path";
    return {};
#endif // QT_CONFIG(opengl) && QT_CONFIG(egl)
}

void QDmaBufVideoBuffer::unmapRegions(QSpan<const MappedRegion> regions)
{
    for (const MappedRegion &region : regions) {
        dmaBufSyncRead(region.fd.get(), DmaBufCpuAccess::End);
        ::munmap(region.data.data(), region.data.size());
    }
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

    for (qsizetype i = 0; i < m_planes.size(); ++i) {
        const DmaBufPlane &plane = m_planes[i];

        auto fd = dupFd(plane.fd);
        if (!fd) {
            qCWarning(qLcDmaBufVideoBuffer)
                    << "Failed to dup DMABUF fd for plane" << i << ':' << fd.error().message();
            unmapRegions(m_mappedRegions);
            m_mappedRegions.clear();
            return {};
        }

        const off_t len = ::lseek(fd->get(), 0, SEEK_END);
        void *mapped = len > 0 ? ::mmap(nullptr, size_t(len), PROT_READ, MAP_SHARED, fd->get(), 0)
                               : MAP_FAILED;
        if (mapped == MAP_FAILED) {
            qCWarning(qLcDmaBufVideoBuffer)
                    << "Could not map DMABUF for plane" << i << ':' << qt_error_string(errno);
            unmapRegions(m_mappedRegions);
            m_mappedRegions.clear();
            return {};
        }

        dmaBufSyncRead(fd->get(), DmaBufCpuAccess::Begin);
        const QSpan<std::byte> data{
            static_cast<std::byte *>(mapped),
            qsizetype(len),
        };
        m_mappedRegions.push_back({
                std::move(*fd),
                data,
        });

        mapData.data[i] = reinterpret_cast<uchar *>(data.data()) + plane.offset;
        mapData.bytesPerLine[i] = int(plane.pitch);
        mapData.dataSize[i] = int(plane.pitch) * desc->heightForPlane(m_size.height(), i);
    }

    mapData.planeCount = m_planes.size();

    m_mapMode = mode;
    return mapData;
}

void QDmaBufVideoBuffer::unmap()
{
    unmapRegions(m_mappedRegions);
    m_mappedRegions.clear();

    m_mapMode = QVideoFrame::NotMapped;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimedia_dmabuf_support_p.h"

#include <QtMultimedia/private/qdmabufallocator_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qtmm_alignment_support_p.h>
#include <QtCore/qloggingcategory.h>

#if QT_CONFIG(linux_dmabuf)
#  include <QtCore/qscopeguard.h>
#  include <linux/dma-buf.h>
#  include <sys/ioctl.h>
#  include <sys/mman.h>
#  include <unistd.h>
#endif

#include <cerrno>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

#if QT_CONFIG(linux_dmabuf)

Q_STATIC_LOGGING_CATEGORY(qLcDmaBufSupport, "qt.multimedia.dmabufsupport");

void dmaBufSyncRead(int fd, DmaBufCpuAccess access)
{
    const uint64_t startOrEnd =
            access == DmaBufCpuAccess::Begin ? DMA_BUF_SYNC_START : DMA_BUF_SYNC_END;

    dma_buf_sync sync = {
        .flags = startOrEnd | DMA_BUF_SYNC_READ,
    };
    if (qt_safe_ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync) < 0)
        qCDebug(qLcDmaBufSupport) << "DMA_BUF_IOCTL_SYNC failed" << qt_error_string(errno);
}

std::optional<QByteArray> copyDmaBufData(int fd, off_t offset, size_t size, size_t bufferSize)
{
    if (offset < 0 || size_t(offset) > bufferSize || size > bufferSize - size_t(offset)) {
        qCWarning(qLcDmaBufSupport) << "DMA-BUF range lies outside its buffer";
        return std::nullopt;
    }

    if (size == 0)
        return QByteArray{};

    // The mapping must start on a page boundary, so map from the page holding offset.
    const int sysconfPageSize = sysconf(_SC_PAGESIZE);
    if (Q_UNLIKELY(sysconfPageSize < 0)) {
        qCWarning(qLcDmaBufSupport) << "cannot determine system page size:" << qt_error_string(errno);
        return std::nullopt;
    }
    const size_t pageSize = size_t(sysconfPageSize);
    const off_t mapOffset = alignDown(offset, pageSize);
    const size_t pageOffset = size_t(offset - mapOffset);

    void *mapping = mmap(nullptr, pageOffset + size, PROT_READ, MAP_SHARED, fd, mapOffset);
    if (mapping == MAP_FAILED) {
        qCWarning(qLcDmaBufSupport) << "mapping DMA-BUF failed:" << qt_error_string(errno);
        return std::nullopt;
    }
    auto unmap = qScopeGuard([&] {
        munmap(mapping, pageOffset + size);
    });

    dmaBufSyncRead(fd, DmaBufCpuAccess::Begin);
    auto syncEnd = qScopeGuard([&] {
        dmaBufSyncRead(fd, DmaBufCpuAccess::End);
    });

    return QByteArray(static_cast<const char *>(mapping) + pageOffset, qsizetype(size));
}

std::optional<QByteArray> copyDmaBufData(int fd, off_t offset, size_t size)
{
    const off_t bufferSize = lseek(fd, 0, SEEK_END);
    if (bufferSize < 0) {
        qCWarning(qLcDmaBufSupport) << "cannot determine DMA-BUF size:" << qt_error_string(errno);
        return std::nullopt;
    }

    return copyDmaBufData(fd, offset, size, size_t(bufferSize));
}

std::optional<QByteArray> copyDmaBufData(const QDmaBuf &buf, off_t offset, size_t size)
{
    return copyDmaBufData(buf.fd(), offset, size, buf.size());
}

#endif // QT_CONFIG(linux_dmabuf)

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

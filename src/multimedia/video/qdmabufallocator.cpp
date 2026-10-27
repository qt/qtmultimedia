// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdmabufallocator_p.h"

#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qtmm_alignment_support_p.h>
#include <QtCore/private/qcore_unix_p.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>

#if __has_include(<linux/dma-heap.h>)
#  include <linux/dma-heap.h>
#  define QT_DMABUF_HAS_DMA_HEAP
#endif
#if __has_include(<linux/udmabuf.h>)
#  include <linux/udmabuf.h>
#  define QT_DMABUF_HAS_UDMABUF
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtMultimediaPrivate {

namespace {

#ifdef QT_DMABUF_HAS_DMA_HEAP
constexpr auto kDmaHeapPaths = std::initializer_list<const char *>{
    "/dev/dma_heap/linux,cma",
    "/dev/dma_heap/reserved",
    "/dev/dma_heap/system",
};
#endif

q23::expected<QUniqueFileDescriptorHandle, QString>
allocFromDmaHeap([[maybe_unused]] int heapFd, [[maybe_unused]] qsizetype size)
{
#ifdef QT_DMABUF_HAS_DMA_HEAP
    dma_heap_allocation_data alloc{
        .len = __u64(size),
        .fd = 0,
        .fd_flags = O_CLOEXEC | O_RDWR,
        .heap_flags = 0,
    };

    if (qt_safe_ioctl(heapFd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
        const int error = errno;
        return q23::unexpected(u"DMA heap allocation failed: %1"_s.arg(qt_error_string(error)));
    }

    return QUniqueFileDescriptorHandle{ int(alloc.fd) };
#else
    return q23::unexpected(u"dma-heap is not supported by the kernel headers"_s);
#endif
}

q23::expected<QUniqueFileDescriptorHandle, QString>
allocFromUDmaBuf([[maybe_unused]] int udmabufFd, [[maybe_unused]] qsizetype size)
{
#ifdef QT_DMABUF_HAS_UDMABUF
    QUniqueFileDescriptorHandle memfd{ memfd_create("qt-dmabuf", MFD_CLOEXEC | MFD_ALLOW_SEALING) };
    if (!memfd.isValid()) {
        const int error = errno;
        return q23::unexpected(u"memfd_create failed: %1"_s.arg(qt_error_string(error)));
    }

    if (ftruncate(memfd.get(), size) < 0) {
        const int error = errno;
        return q23::unexpected(u"ftruncate failed: %1"_s.arg(qt_error_string(error)));
    }

    // udmabuf requires the memfd to be sealed against shrinking.
    if (fcntl(memfd.get(), F_ADD_SEALS, F_SEAL_SHRINK) < 0) {
        const int error = errno;
        return q23::unexpected(u"sealing memfd failed: %1"_s.arg(qt_error_string(error)));
    }

    udmabuf_create create{
        .memfd = __u32(memfd.get()),
        .flags = UDMABUF_FLAGS_CLOEXEC,
        .offset = 0,
        .size = __u64(size),
    };

    // The kernel keeps its own reference to the memfd, so it may be closed afterwards.
    QUniqueFileDescriptorHandle buffer{ qt_safe_ioctl(udmabufFd, UDMABUF_CREATE, &create) };
    if (!buffer.isValid()) {
        const int error = errno;
        return q23::unexpected(u"UDMABUF_CREATE failed: %1"_s.arg(qt_error_string(error)));
    }

    return buffer;
#else
    return q23::unexpected(u"udmabuf is not supported by the kernel headers"_s);
#endif
}

} // namespace

QDmaBuf::QDmaBuf(QUniqueFileDescriptorHandle fd, qsizetype size) : m_fd(std::move(fd)), m_size(size)
{
}

QDmaBufAllocator::QDmaBufAllocator(QUniqueFileDescriptorHandle providerFd, Provider provider)
    : m_providerFd(std::move(providerFd)), m_provider(provider)
{
}

q23::expected<QDmaBufAllocator, QString> QDmaBufAllocator::create()
{
#ifdef QT_DMABUF_HAS_DMA_HEAP
    for (const char *path : kDmaHeapPaths) {
        QUniqueFileDescriptorHandle fd{ qt_safe_open(path, O_RDONLY | O_CLOEXEC) };
        if (fd.isValid())
            return QDmaBufAllocator{ std::move(fd), Provider::DmaHeap };
    }
#endif

#ifdef QT_DMABUF_HAS_UDMABUF
    QUniqueFileDescriptorHandle fd{ qt_safe_open("/dev/udmabuf", O_RDWR | O_CLOEXEC) };
    if (fd.isValid())
        return QDmaBufAllocator{ std::move(fd), Provider::UDmaBuf };
#endif

    return q23::unexpected(u"no DMA-BUF provider (dma-heap or udmabuf) is accessible"_s);
}

q23::expected<std::shared_ptr<QDmaBuf>, QString> QDmaBufAllocator::allocate(qsizetype size) const
{
    static const long pageSize = sysconf(_SC_PAGESIZE);
    const qsizetype alignedSize = QtMultimediaPrivate::alignUp(size, size_t(pageSize));

    auto fd = m_provider == Provider::DmaHeap ? allocFromDmaHeap(m_providerFd.get(), alignedSize)
                                              : allocFromUDmaBuf(m_providerFd.get(), alignedSize);
    if (!fd)
        return q23::unexpected(std::move(fd.error()));

    return std::make_shared<QDmaBuf>(std::move(*fd), alignedSize);
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

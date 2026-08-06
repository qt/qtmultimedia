// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Needed for F_ADD_SEALS/F_SEAL_SHRINK (fcntl.h) and memfd_create (sys/mman.h).
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "udmabuftestutils_p.h"

#include <fcntl.h>
#include <linux/memfd.h>
#include <linux/udmabuf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

quint64 udmabufPageAlignedSize(quint64 size)
{
    const quint64 pageSize = ::sysconf(_SC_PAGESIZE);
    return (size + pageSize - 1) / pageSize * pageSize;
}

QUniqueFileDescriptorHandle createUdmabufTestMemfd(const QByteArray &data, quint64 bufferSize,
                                                   const char *debugName)
{
    QUniqueFileDescriptorHandle memfd{
        ::memfd_create(debugName, MFD_ALLOW_SEALING),
    };
    if (!memfd)
        return {};
    if (::ftruncate(memfd.get(), bufferSize) != 0)
        return {};
    if (::write(memfd.get(), data.constData(), data.size()) != data.size())
        return {};
    if (::fcntl(memfd.get(), F_ADD_SEALS, F_SEAL_SHRINK) != 0)
        return {};
    return memfd;
}

QUniqueFileDescriptorHandle createUdmabufFd(const QUniqueFileDescriptorHandle &memFd, quint64 size)
{
    QUniqueFileDescriptorHandle device{
        ::open("/dev/udmabuf", O_RDWR),
    };
    if (!device)
        return {};

    udmabuf_create create{
        .memfd = uint32_t(memFd.get()),
        .flags = UDMABUF_FLAGS_CLOEXEC,
        .offset = 0,
        .size = size,
    };

    QUniqueFileDescriptorHandle dmabufFd{
        ::ioctl(device.get(), UDMABUF_CREATE, &create),
    };
    return dmabufFd;
}

bool canUseUdmabuf()
{
    return ::access("/dev/udmabuf", R_OK | W_OK) == 0;
}

QT_END_NAMESPACE

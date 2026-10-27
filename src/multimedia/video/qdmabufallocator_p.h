// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDMABUFALLOCATOR_P_H
#define QDMABUFALLOCATOR_P_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

// A DMA-BUF that owns its file descriptor. It does not refer to the allocator that
// created it, so it can outlive the allocator.
class Q_MULTIMEDIA_EXPORT QDmaBuf
{
public:
    QDmaBuf(QUniqueFileDescriptorHandle fd, qsizetype size);

    int fd() const { return m_fd.get(); }
    qsizetype size() const { return m_size; }

private:
    QUniqueFileDescriptorHandle m_fd;
    const qsizetype m_size = 0;
};

class Q_MULTIMEDIA_EXPORT QDmaBufAllocator
{
public:
    static q23::expected<QDmaBufAllocator, QString> create();

    q23::expected<std::shared_ptr<QDmaBuf>, QString> allocate(qsizetype size) const;

private:
    enum class Provider : uint8_t {
        DmaHeap, // /dev/dma_heap/system, may not be accessible as user
        UDmaBuf,
    };

    QDmaBufAllocator(QUniqueFileDescriptorHandle providerFd, Provider provider);

    QUniqueFileDescriptorHandle m_providerFd;
    const Provider m_provider;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QDMABUFALLOCATOR_P_H

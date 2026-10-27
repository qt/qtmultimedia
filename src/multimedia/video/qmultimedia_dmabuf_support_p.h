// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIA_DMABUF_SUPPORT_P_H
#define QMULTIMEDIA_DMABUF_SUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#if QT_CONFIG(linux_dmabuf)
#  include <sys/types.h>
#endif

#include <cstddef>
#include <cstdint>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

#if QT_CONFIG(linux_dmabuf)

class QDmaBuf;

enum class DmaBufCpuAccess : uint8_t {
    Begin,
    End,
};

Q_MULTIMEDIA_EXPORT void dmaBufSyncRead(int fd, DmaBufCpuAccess access);

// Copies bytes [offset, offset + size) of a DMA-BUF, validating the range against the
// size of the DMA-BUF.
Q_MULTIMEDIA_EXPORT std::optional<QByteArray> copyDmaBufData(int fd, off_t offset, size_t size);
Q_MULTIMEDIA_EXPORT std::optional<QByteArray> copyDmaBufData(const QDmaBuf &, off_t offset,
                                                             size_t size);

// Copies bytes [offset, offset + size) of a DMA-BUF, validating the range against bufferSize.
Q_MULTIMEDIA_EXPORT std::optional<QByteArray> copyDmaBufData(int fd, off_t offset, size_t size,
                                                             size_t bufferSize);

#endif // QT_CONFIG(linux_dmabuf)

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QMULTIMEDIA_DMABUF_SUPPORT_P_H

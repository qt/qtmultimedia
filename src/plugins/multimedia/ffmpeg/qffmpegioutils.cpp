// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegioutils_p.h"
#include "qiodevice.h"
#include "qffmpegdefs_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

int readQIODevice(void *opaque, uint8_t *buf, int buf_size)
{
    auto *dev = static_cast<QIODevice *>(opaque);
    Q_ASSERT(dev);

    if (dev->atEnd())
        return AVERROR_EOF;
    return dev->read(reinterpret_cast<char *>(buf), buf_size);
}

int writeQIODevice(void *opaque, AvioWriteBufferType buf, int buf_size)
{
    auto dev = static_cast<QIODevice *>(opaque);
    Q_ASSERT(dev);

    return dev->write(reinterpret_cast<const char *>(buf), buf_size);
}

int64_t seekQIODevice(void *opaque, int64_t offset, int whence)
{
    QIODevice *dev = static_cast<QIODevice *>(opaque);
    Q_ASSERT(dev);

    if (dev->isSequential())
        return AVERROR(EINVAL);

    if (whence & AVSEEK_SIZE)
        return dev->size();

    whence &= ~AVSEEK_FORCE;

    if (whence == SEEK_CUR)
        offset += dev->pos();
    else if (whence == SEEK_END)
        offset += dev->size();

    if (!dev->seek(offset))
        return AVERROR(EINVAL);
    return offset;
}

} // namespace QFFmpeg

QT_END_NAMESPACE

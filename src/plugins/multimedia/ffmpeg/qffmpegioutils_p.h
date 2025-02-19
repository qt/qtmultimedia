// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGIOUTILS_P_H
#define QFFMPEGIOUTILS_P_H

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

#include "qtmultimediaglobal.h"
#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

int readQIODevice(void *opaque, uint8_t *buf, int buf_size);

using AvioWriteBufferType =
        std::conditional_t<QT_FFMPEG_AVIO_WRITE_CONST, const uint8_t *, uint8_t *>;

int writeQIODevice(void *opaque, AvioWriteBufferType buf, int buf_size);

int64_t seekQIODevice(void *opaque, int64_t offset, int whence);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGIOUTILS_P_H

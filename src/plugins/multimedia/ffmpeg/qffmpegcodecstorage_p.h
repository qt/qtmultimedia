// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGCODECSTORAGE_P_H
#define QFFMPEGCODECSTORAGE_P_H

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

#include "qffmpegdefs_p.h"
#include "qtmultimediaglobal.h"

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

bool findAndOpenAVDecoder(AVCodecID codecId,
                          const std::function<AVScore(const AVCodec *)> &scoresGetter,
                          const std::function<bool(const AVCodec *)> &codecOpener);

bool findAndOpenAVEncoder(AVCodecID codecId,
                          const std::function<AVScore(const AVCodec *)> &scoresGetter,
                          const std::function<bool(const AVCodec *)> &codecOpener);

const AVCodec *findAVDecoder(AVCodecID codecId,
                             const std::optional<PixelOrSampleFormat> &format = {});

const AVCodec *findAVEncoder(AVCodecID codecId,
                             const std::optional<PixelOrSampleFormat> &format = {});

const AVCodec *findAVEncoder(AVCodecID codecId,
                             const std::function<AVScore(const AVCodec *)> &scoresGetter);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGCODECSTORAGE_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h>
#include "qtmultimediaglobal.h"

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {
class Codec;

bool findAndOpenAVDecoder(AVCodecID codecId,
                          const std::function<AVScore(const Codec &)> &scoresGetter,
                          const std::function<bool(const Codec &)> &codecOpener);

bool findAndOpenAVEncoder(AVCodecID codecId,
                          const std::function<AVScore(const Codec &)> &scoresGetter,
                          const std::function<bool(const Codec &)> &codecOpener);

std::optional<Codec> findAVDecoder(AVCodecID codecId,
                                   const std::optional<PixelOrSampleFormat> &format = {});

std::optional<Codec> findAVEncoder(AVCodecID codecId,
                                   const std::optional<PixelOrSampleFormat> &format = {});

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGCODECSTORAGE_P_H

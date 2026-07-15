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
#include <QtFFmpegMediaPluginImpl/private/qffmpegcodec_p.h>
#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtCore/qxpfunctional.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

enum class CodecRole : uint8_t {
    Encoders,
    Decoders,
};

struct CodecScoreRecord
{
    Codec codec;
    AVScore score;
};

std::vector<CodecScoreRecord>
findAndScoreCodecs(CodecRole, AVCodecID,
                   const qxp::function_ref<AVScore(const Codec &)> &scoreFunction);

// note: Sorted by score descending, so the first element is the best match.
inline std::vector<CodecScoreRecord>
findAndScoreEncoders(AVCodecID codecId,
                     const qxp::function_ref<AVScore(const Codec &)> &scoreFunction)
{
    return findAndScoreCodecs(CodecRole::Encoders, codecId, scoreFunction);
}

std::optional<Codec> findAVDecoder(AVCodecID codecId,
                                   const std::optional<PixelOrSampleFormat> &format = {});

std::optional<Codec> findAVEncoder(AVCodecID codecId,
                                   const std::optional<PixelOrSampleFormat> &format = {});

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGCODECSTORAGE_P_H

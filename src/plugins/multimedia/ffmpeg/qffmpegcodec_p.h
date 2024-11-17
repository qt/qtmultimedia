// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGCODEC_P_H
#define QFFMPEGCODEC_P_H

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

#include "qffmpegdefs_p.h" // Important: Must be included first

#include <QtCore/qlatin1stringview.h>

extern "C" {
#include <libavcodec/codec.h>
}

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class Codec
{
public:
    Codec() = default;
    explicit Codec(const AVCodec *codec);

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] const AVCodec *get() const noexcept;
    [[nodiscard]] AVCodecID id() const noexcept;
    [[nodiscard]] QLatin1StringView name() const noexcept;
    [[nodiscard]] AVMediaType type() const noexcept;
    [[nodiscard]] int capabilities() const noexcept;
    [[nodiscard]] bool isEncoder() const noexcept;
    [[nodiscard]] bool isDecoder() const noexcept;
    [[nodiscard]] bool isExperimental() const noexcept;
    [[nodiscard]] const AVPixelFormat *pixelFormats() const noexcept;
    [[nodiscard]] const AVSampleFormat *sampleFormats() const noexcept;
    [[nodiscard]] const int *sampleRates() const noexcept;
    [[nodiscard]] const ChannelLayoutT *channelLayouts() const noexcept;
    [[nodiscard]] const AVRational *frameRates() const noexcept;
    [[nodiscard]] const AVCodecHWConfig *hwConfig(int i) const noexcept;

private:
    const AVCodec *m_codec = nullptr;
};

// Minimal iterator to support range-based for-loop
class CodecIterator
{
public:
    // named constructors
    static CodecIterator begin();
    static CodecIterator end();

    CodecIterator &operator++() noexcept;
    [[nodiscard]] Codec operator*() const noexcept;
    [[nodiscard]] bool operator!=(const CodecIterator &other) const noexcept;

private:
    void *m_state = nullptr;
    const AVCodec *m_codec = nullptr;
};

using CodecEnumerator = CodecIterator;

} // namespace FFmpeg

QT_END_NAMESPACE

#endif

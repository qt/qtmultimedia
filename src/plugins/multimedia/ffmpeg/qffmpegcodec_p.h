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

#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h> // Important: Must be included first

#include <QtCore/qlatin1stringview.h>
#include <QtCore/qspan.h>

#include <vector>

extern "C" {
#include <libavcodec/codec.h>
}

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class Codec
{
public:
    explicit Codec(const AVCodec *codec);

    [[nodiscard]] const AVCodec *get() const noexcept;
    [[nodiscard]] AVCodecID id() const noexcept;
    [[nodiscard]] QLatin1StringView name() const noexcept;
    [[nodiscard]] AVMediaType type() const noexcept;
    [[nodiscard]] int capabilities() const noexcept;
    [[nodiscard]] bool isEncoder() const noexcept;
    [[nodiscard]] bool isDecoder() const noexcept;
    [[nodiscard]] bool isExperimental() const noexcept;
    [[nodiscard]] QSpan<const AVPixelFormat> pixelFormats() const noexcept;
    [[nodiscard]] QSpan<const AVSampleFormat> sampleFormats() const noexcept;
    [[nodiscard]] QSpan<const int> sampleRates() const noexcept;
    [[nodiscard]] QSpan<const ChannelLayoutT> channelLayouts() const noexcept;
    [[nodiscard]] QSpan<const AVRational> frameRates() const noexcept;
    [[nodiscard]] std::vector<const AVCodecHWConfig *> hwConfigs() const noexcept;

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

// Helper function to wrap pixel formats inside a span
QSpan<const AVPixelFormat> makeSpan(const AVPixelFormat *values);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDRAWAVAUDIODECODER_P_H
#define QDRAWAVAUDIODECODER_P_H

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

#include <QtMultimedia/private/qplatformaudiodecoder_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qspan.h>
#include <QtCore/qurl.h>
#include <QtCore/private/qexpected_p.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>

QT_BEGIN_NAMESPACE

class QAudioDecoder;
class QIODevice;

namespace QtMultimediaPrivate {

using QDrWavDecodeResult = q23::expected<QAudioBuffer, std::pair<QAudioDecoder::Error, QString>>;
QDrWavDecodeResult loadWaveAndDecodeData(QSpan<const std::byte>,
                                         const QAudioFormat &requestedFormat);

#ifdef __cpp_concepts
template <typename Reader>
concept FrameReader = std::is_invocable_v<Reader, uint64_t, char *>
        && std::is_convertible_v<std::invoke_result_t<Reader, uint64_t, char *>, uint64_t>;
#endif

#ifdef __cpp_concepts
template <FrameReader Reader>
#else
template <typename Reader>
#endif
std::optional<QByteArray> readFramesInChunks(uint64_t declaredFrameCount, size_t bytesPerFrame,
                                             Reader &&readFrames)
{
    constexpr size_t chunkBytes = 10 * 1024 * 1024; // 10 MB
    if (bytesPerFrame == 0)
        return std::nullopt;

    const uint64_t chunkFrames = std::max<uint64_t>(1, chunkBytes / bytesPerFrame);

    QT_TRY
    {
        QByteArray result;
        uint64_t totalFramesRead = 0;
        while (totalFramesRead < declaredFrameCount) {
            const uint64_t framesToRead =
                    std::min(chunkFrames, declaredFrameCount - totalFramesRead);
            const uint64_t bytesToRead = framesToRead * bytesPerFrame;
            const qsizetype oldSize = result.size();
            if (bytesToRead > uint64_t(QByteArray::maxSize() - oldSize))
                return std::nullopt;

            result.resizeForOverwrite(oldSize + qsizetype(bytesToRead));
            const uint64_t framesRead = readFrames(framesToRead, result.data() + oldSize);
            Q_ASSERT(framesRead <= framesToRead);
            result.resize(oldSize + qsizetype(framesRead * bytesPerFrame));

            totalFramesRead += framesRead;
            if (framesRead < framesToRead)
                break;
        }
        if (totalFramesRead != declaredFrameCount)
            return std::nullopt;
        return result;
    }
    QT_CATCH(const std::bad_alloc &)
    {
        return std::nullopt;
    }
}

} // namespace QtMultimediaPrivate

class QDrWavAudioDecoder final : public QPlatformAudioDecoder
{
public:
    explicit QDrWavAudioDecoder(QAudioDecoder *parent);
    ~QDrWavAudioDecoder() override;

    // Source management
    QUrl source() const override { return m_source; }
    void setSource(const QUrl &) override;

    QIODevice *sourceDevice() const override { return m_sourceDevice; }
    void setSourceDevice(QIODevice *) override;

    // Lifecycle
    void start() override;
    void stop() override;

    // Format
    QAudioFormat audioFormat() const override { return m_audioFormat; }
    void setAudioFormat(const QAudioFormat &) override;

    // Buffer pull model
    QAudioBuffer read() override;

    // Metadata
    bool canReadQrc() const override { return true; }

private:
    using QDrWavDecodeResult = QtMultimediaPrivate::QDrWavDecodeResult;

    QDrWavDecodeResult loadAndDecodeFile(const QUrl &, QIODevice *, const QAudioFormat &,
                                         std::shared_ptr<std::atomic_bool> decodingStopped);

    // Handle decode completion
    void onDecodeFinished(QDrWavDecodeResult, const std::atomic_bool &decodingStopped);

    QUrl m_source;
    QIODevice *m_sourceDevice = nullptr;
    QAudioFormat m_audioFormat;

    // Decoded result
    QAudioBuffer m_buffer;

    std::shared_ptr<std::atomic_bool> m_decodingStopped;
};

QT_END_NAMESPACE

#endif // QDRAWAVAUDIODECODER_P_H

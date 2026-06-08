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
#include <QtCore/qspan.h>
#include <QtCore/qurl.h>
#include <QtCore/private/qexpected_p.h>

#include <atomic>
#include <memory>

QT_BEGIN_NAMESPACE

class QAudioDecoder;
class QIODevice;

namespace QtMultimediaPrivate {

using QDrWavDecodeResult = q23::expected<QAudioBuffer, std::pair<QAudioDecoder::Error, QString>>;
QDrWavDecodeResult loadWaveAndDecodeData(QSpan<const std::byte>,
                                         const QAudioFormat &requestedFormat);

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

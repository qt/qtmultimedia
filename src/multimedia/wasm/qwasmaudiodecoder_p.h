// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMAUDIODECODER_P_H
#define QWASMAUDIODECODER_P_H

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

#include <private/qplatformaudiodecoder_p.h>
#include <QtMultimedia/private/qaudio_parsing_support_p.h>
#include <QtCore/qqueue.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>
#include <QtMultimedia/qaudiobuffer.h>

#include <emscripten/val.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QWasmAudioDecoder final : public QPlatformAudioDecoder
{
    Q_OBJECT

public:
    explicit QWasmAudioDecoder(QAudioDecoder *parent);
    ~QWasmAudioDecoder() override;

    QUrl source() const override;
    void setSource(const QUrl &url) override;
    QIODevice *sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;
    void start() override;
    void stop() override;
    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;
    QAudioBuffer read() override;
    bool canReadQrc() const override { return true; }

private:
    using Codec = QtMultimediaPrivate::ParsingSupport::AudioCodec;

    void feedChunk(const QByteArray &chunk);
    void feedAvailableFrames(bool endOfInput);
    void beginDecoding(Codec codec, const QByteArray &rawHeader);
    void onSourceReadyRead();
    void onSourceDone();
    void startFlush();
    void deliverOutputs();
    void tryDecodeAudioData();
    QString decorateWithFallbackReason(const QString &message) const;
    void closeDecoder();
    void resetDecodingState();
    void handleError(QAudioDecoder::Error code, const QString &message);

    QUrl m_source;
    QIODevice *m_sourceDevice = nullptr;
    QAudioFormat m_audioFormat;
    QQueue<QAudioBuffer> m_pendingBuffers;
    emscripten::val m_decoder = emscripten::val::undefined();
    const std::shared_ptr<int> m_generationPtr = std::make_shared<int>(0);
    QByteArray m_inputBuffer;
    QByteArray m_rawData;
    QString m_fallbackReason;
    // Drains decoded outputs while the decoder runs, so buffers reach the
    // caller as they are decoded instead of all at once when flush() resolves.
    QTimer m_outputDrainTimer;
    Codec m_codec = Codec::Unknown;
    double m_byteOffset = 0.0;
    // Total PCM frames delivered so far, used to derive each buffer's
    // presentation time. The timestamps fed to EncodedAudioChunk are synthetic
    // byte offsets, so the value that comes back on the AudioData is not a
    // usable presentation time.
    qint64 m_decodedFrames = 0;
    qsizetype m_inputPos = 0;
    bool m_checkingSupport = false;
    bool m_configured = false;
    bool m_flushing = false;
    bool m_formatEmitted = false;
    bool m_useDecodeAudioData = false;
    // Set while deliverOutputs() is running, so a nested event loop started
    // from a bufferReady() slot cannot process the same outputs twice.
    bool m_deliveringOutputs = false;
    // FLAC only: whether m_inputPos has been moved past the container header
    // and metadata blocks, which must not be scanned for frame sync codes.
    bool m_flacHeaderSkipped = false;
};

QT_END_NAMESPACE

#endif // QWASMAUDIODECODER_P_H

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdrawavaudiodecoder_p.h"

#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qfile.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthreadpool.h>

#if QT_CONFIG(network)
#  include <QtNetwork/qnetworkaccessmanager.h>
#  include <QtNetwork/qnetworkreply.h>
#  include <QtNetwork/qnetworkrequest.h>
#endif

#include <dr_wav.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcDrWavDecoder, "qt.multimedia.drwavdecoder")

namespace QtMultimediaPrivate {

static QDrWavDecodeResult makeFormatError(const QString &message)
{
    return q23::unexpected(std::pair(QAudioDecoder::FormatError, message));
}

static QDrWavDecodeResult makeResourceError(const QString &message)
{
    return q23::unexpected(std::pair(QAudioDecoder::ResourceError, message));
}

QDrWavDecodeResult loadWaveAndDecodeData(QSpan<const std::byte> data,
                                         const QAudioFormat &requestedFormat)
{
    using namespace QtPrivate;

    drwav wav;
    if (!drwav_init_memory(&wav, data.data(), data.size(), nullptr)) {
        qCDebug(qLcDrWavDecoder) << "Failed to initialize dr_wav decoder";
        return makeFormatError(QAudioDecoder::tr("Unable to decode audio file"));
    }

    auto cleanup = qScopeGuard([&] {
        drwav_uninit(&wav);
    });

    // Determine output format
    QAudioFormat outputFormat;
    size_t bytesPerFrame;

    if (requestedFormat.isValid()) {
        // User set a format - check compatibility with file
        if (requestedFormat.sampleRate() != static_cast<int>(wav.sampleRate))
            return makeFormatError(
                    QAudioDecoder::tr("Audio file sample rate does not match requested format"));
        if (requestedFormat.channelCount() != static_cast<int>(wav.channels))
            return makeFormatError(
                    QAudioDecoder::tr("Audio file channel count does not match requested format"));
        outputFormat = requestedFormat;

        // Determine bytes per frame based on requested format
        switch (requestedFormat.sampleFormat()) {
        case QAudioFormat::UInt8:
            bytesPerFrame = wav.channels * sizeof(uint8_t);
            break;
        case QAudioFormat::Int16:
            bytesPerFrame = wav.channels * sizeof(int16_t);
            break;
        case QAudioFormat::Int32:
            bytesPerFrame = wav.channels * sizeof(int32_t);
            break;
        case QAudioFormat::Float:
            bytesPerFrame = wav.channels * sizeof(float);
            break;
        default:
            return makeFormatError(QAudioDecoder::tr("Unsupported sample format"));
        }
    } else {
        // Infer format from file header - use native format heuristic
        QAudioFormat::SampleFormat sampleFormat;

        switch (wav.bitsPerSample) {
        case 8:
            sampleFormat = QAudioFormat::UInt8;
            bytesPerFrame = wav.channels * sizeof(uint8_t);
            break;
        case 16:
            sampleFormat = QAudioFormat::Int16;
            bytesPerFrame = wav.channels * sizeof(int16_t);
            break;
        case 24: // 24-bit → read as Int32
            sampleFormat = QAudioFormat::Int32;
            bytesPerFrame = wav.channels * sizeof(int32_t);
            break;
        case 32:
            sampleFormat = QAudioFormat::Int32;
            bytesPerFrame = wav.channels * sizeof(int32_t);
            break;
        default: // unsupported → fallback Float
            sampleFormat = QAudioFormat::Float;
            bytesPerFrame = wav.channels * sizeof(float);
            break;
        }

        outputFormat.setChannelCount(wav.channels);
        outputFormat.setSampleFormat(sampleFormat);
        outputFormat.setSampleRate(wav.sampleRate);
        outputFormat.setChannelConfig(
                QAudioFormat::defaultChannelConfigForChannelCount(wav.channels));
    }

    qCDebug(qLcDrWavDecoder) << "Decoded WAV:"
                             << "channels=" << wav.channels << "sampleRate=" << wav.sampleRate
                             << "bitsPerSample=" << wav.bitsPerSample
                             << "totalPCMFrameCount=" << wav.totalPCMFrameCount
                             << "outputFormat=" << outputFormat.sampleFormat();

    // Allocate PCM data for the entire file
    QByteArray pcmData;
    pcmData.resizeForOverwrite(wav.totalPCMFrameCount * bytesPerFrame);

    // Read all frames at once using appropriate format
    uint64_t framesRead = 0;

    switch (outputFormat.sampleFormat()) {
    case QAudioFormat::UInt8:
        framesRead = drwav_read_pcm_frames(&wav, wav.totalPCMFrameCount,
                                           reinterpret_cast<uint8_t *>(pcmData.data()));
        break;
    case QAudioFormat::Int16:
        framesRead = drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount,
                                               reinterpret_cast<int16_t *>(pcmData.data()));
        break;
    case QAudioFormat::Int32:
        framesRead = drwav_read_pcm_frames_s32(&wav, wav.totalPCMFrameCount,
                                               reinterpret_cast<int32_t *>(pcmData.data()));
        break;
    case QAudioFormat::Float:
        framesRead = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount,
                                               reinterpret_cast<float *>(pcmData.data()));
        break;
    default:
        return makeFormatError(QAudioDecoder::tr("Unsupported sample format"));
    }

    if (framesRead != wav.totalPCMFrameCount) {
        qCDebug(qLcDrWavDecoder) << "Failed to read all frames:"
                                 << "expected" << wav.totalPCMFrameCount << "got" << framesRead;
        return makeFormatError(QAudioDecoder::tr("Unable to read audio data"));
    }

    QAudioBuffer buffer(pcmData, outputFormat);
    if (!buffer.isValid()) {
        return makeFormatError(QAudioDecoder::tr("Failed to create audio buffer"));
    }

    qCDebug(qLcDrWavDecoder) << "Successfully decoded:" << buffer.sampleCount() << "samples";

    return buffer;
}

} // namespace QtMultimediaPrivate

QDrWavAudioDecoder::QDrWavAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
{
}

QDrWavAudioDecoder::~QDrWavAudioDecoder()
{
    stop();
}

void QDrWavAudioDecoder::setSource(const QUrl &fileName)
{
    stop();
    m_sourceDevice = nullptr;
    if (std::exchange(m_source, fileName) != fileName)
        sourceChanged();
}

void QDrWavAudioDecoder::setSourceDevice(QIODevice *device)
{
    stop();
    m_source.clear();
    if (std::exchange(m_sourceDevice, device) != device)
        sourceChanged();
}

void QDrWavAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (isDecoding())
        return;
    m_audioFormat = format;
}


void QDrWavAudioDecoder::start()
{
    if (isDecoding())
        return;

    if (m_source.isEmpty() && !m_sourceDevice) {
        error(QAudioDecoder::ResourceError, tr("No audio source specified"));
        return;
    }

    m_decodingStopped = std::make_shared<std::atomic_bool>(false);

    auto threadPool = QThreadPool::globalInstance();
    threadPool->start([source = m_source, sourceDevice = m_sourceDevice,
                       requestedFormat = m_audioFormat, decodingStopped = m_decodingStopped, this] {
        auto state = loadAndDecodeFile(source, sourceDevice, requestedFormat, decodingStopped);

        QMetaObject::invokeMethod(this,
                                  [this, decodingStopped, state = std::move(state)]() mutable {
            if (*decodingStopped) {
                qCDebug(qLcDrWavDecoder) << "Decoding was stopped, discarding result";
                return;
            }
            onDecodeFinished(std::move(state), *decodingStopped);
        });
    });
}

void QDrWavAudioDecoder::stop()
{
    if (!isDecoding())
        return;

    qCDebug(qLcDrWavDecoder) << "stop() called";

    // Prevent the continuation from delivering the buffer
    if (m_decodingStopped) {
        *m_decodingStopped = true;
        m_decodingStopped.reset();
    }

    m_buffer = QAudioBuffer();
    finished();
}

QAudioBuffer QDrWavAudioDecoder::read()
{
    if (m_buffer.isValid()) {
        QAudioBuffer buffer = std::exchange(m_buffer, QAudioBuffer());
        positionChanged(duration());
        bufferAvailableChanged(false);
        return buffer;
    }
    return QAudioBuffer();
}

auto QDrWavAudioDecoder::loadAndDecodeFile(const QUrl &source, QIODevice *sourceDevice,
                                           const QAudioFormat &requestedFormat,
                                           std::shared_ptr<std::atomic_bool> decodingStopped)
        -> QDrWavDecodeResult
{
    using namespace QtMultimediaPrivate;

    QByteArray fileData;

    // Load file data based on source type
    if (!source.isEmpty()) {
        QString scheme = source.scheme();

        if (scheme.isEmpty() || scheme == u"file") {
            // Local file
            QString filePath = source.isLocalFile() ? source.toLocalFile()
                                                    : source.path();
            QFile file(filePath);
            if (!file.open(QFile::ReadOnly))
                return makeResourceError(QAudioDecoder::tr("Cannot open audio file"));
            fileData = file.readAll();
        } else if (scheme == u"qrc") {
            // QRC resource
            QString filePath = u":" + source.toString(QUrl::RemoveScheme);
            QFile file(filePath);
            if (!file.open(QFile::ReadOnly))
                return makeResourceError(QAudioDecoder::tr("Cannot open audio resource"));
            fileData = file.readAll();
        }
#if QT_CONFIG(network)
        else {
            // Network URL
            QNetworkAccessManager networkAccessManager;
            QNetworkReply *reply = networkAccessManager.get(QNetworkRequest(source));

            if (reply->error() != QNetworkReply::NoError) {
                reply->deleteLater();
                return makeResourceError(QAudioDecoder::tr("Failed to download audio file"));
            }

            // Wait for network reply to finish
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            QObject::connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);

            loop.exec();

            if (reply->error() != QNetworkReply::NoError) {
                reply->deleteLater();
                return makeResourceError(QAudioDecoder::tr("Network error while downloading audio"));
            }

            if (*decodingStopped) {
                reply->deleteLater();
                return QAudioBuffer{};
            }

            fileData = reply->readAll();
            reply->deleteLater();
        }
#endif
    } else if (sourceDevice) {
        if (!sourceDevice->isOpen()) {
            if (!sourceDevice->open(QIODevice::ReadOnly))
                return makeResourceError(QAudioDecoder::tr("Cannot open audio device"));
        }
        fileData = sourceDevice->readAll();
    } else {
        return makeResourceError(QAudioDecoder::tr("No audio source specified"));
    }

    if (fileData.isEmpty())
        return makeResourceError(QAudioDecoder::tr("Audio file is empty"));

    if (*decodingStopped)
        return QAudioBuffer{};

    QMetaObject::invokeMethod(this, [this, decodingStopped] {
        if (!*decodingStopped)
            setIsDecoding(true);
    }, Qt::QueuedConnection);

    return QtMultimediaPrivate::loadWaveAndDecodeData(
            as_bytes(QSpan{ fileData.constData(), fileData.size() }), requestedFormat);
}

void QDrWavAudioDecoder::onDecodeFinished(QDrWavDecodeResult result,
                                          const std::atomic_bool &decodingStopped)
{
    if (!result.has_value()) {
        auto [errorCode, errorString] = result.error();
        error(errorCode, errorString);
        setIsDecoding(false);
        return;
    }

    m_buffer = std::move(*result);
    durationChanged(m_buffer.duration() / 1000); // convert from us to ms
    positionChanged(0);

    bufferAvailableChanged(true);
    bufferReady();
    if (decodingStopped)
        return;

    finished();
}

QT_END_NAMESPACE

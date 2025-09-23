// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiosink_p.h"

#include "qandroidaudioutil_p.h"

#include <QtMultimedia/private/qaudiohelpers_p.h>

QT_BEGIN_NAMESPACE

namespace QtAAudio {

Q_STATIC_LOGGING_CATEGORY(qLcAndroidAudioSink, "qt.multimedia.android.audiosink")

QAndroidAudioSinkStream::QAndroidAudioSinkStream(QAudioDevice device, const QAudioFormat &format,
                                                 std::optional<qsizetype> ringbufferSize,
                                                 QAndroidAudioSink *parent, float volume,
                                                 std::optional<int32_t> hardwareBufferFrames,
                                                 AudioEndpointRole role)
    : QtMultimediaPrivate::QPlatformAudioSinkStream(std::move(device), format, ringbufferSize,
                                                    hardwareBufferFrames, volume),
      m_parent(parent),
      m_role(role)
{
    QtAAudio::StreamBuilder builder(format);

    qCDebug(qLcAndroidAudioSink) << "Creating sink for device id:" << m_audioDevice.id()
                                 << ", description:" << m_audioDevice.description();

    // NOTE: Don't set device when creating a stream for the default bluetooth device
    if (!QAndroidAudioUtil::isDefaultBluetoothDevice(m_audioDevice))
        builder.deviceId = m_audioDevice.id().toInt();

    // Set buffer parameters
    builder.bufferCapacity = m_hardwareBufferFrames ? *m_hardwareBufferFrames : 1024;

    // NOTE: AAudio doesn't support UINT8, so convert to INT16 if that's requested
    if (format.sampleFormat() == QAudioFormat::UInt8)
        m_nativeSampleFormat = NativeSampleFormat::int16_t;

    // Set builder parameters for audio sink
    builder.params.sharingMode = AAUDIO_SHARING_MODE_SHARED;
    builder.params.direction = AAUDIO_DIRECTION_OUTPUT;
    switch (m_role) {
    case QtMultimediaPrivate::AudioEndpointRole::SoundEffect:
        builder.params.outputUsage = AAUDIO_USAGE_GAME;
        builder.params.outputContentType = AAUDIO_CONTENT_TYPE_SONIFICATION;
        break;
    case QtMultimediaPrivate::AudioEndpointRole::Accessibility:
        builder.params.outputUsage = AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY;
        builder.params.outputContentType = AAUDIO_CONTENT_TYPE_SPEECH;
        break;
    case QtMultimediaPrivate::AudioEndpointRole::MediaPlayback:
    case QtMultimediaPrivate::AudioEndpointRole::Other:
        builder.params.outputUsage = AAUDIO_USAGE_MEDIA;
        builder.params.outputContentType = AAUDIO_CONTENT_TYPE_MUSIC;
        break;
    }

    builder.userData = this;
    builder.callback = [](AAudioStream *, void *userData, void *audioData,
                          int32_t numFrames) -> int {
        auto *stream = reinterpret_cast<QAndroidAudioSinkStream *>(userData);
        Q_ASSERT(stream);
        return (stream->m_audioCallback) ? stream->processCallback(audioData, numFrames)
                                         : stream->processRingbuffer(audioData, numFrames);
    };
    builder.errorCallback = [](AAudioStream *, void *userData, aaudio_result_t error) -> void {
        auto *stream = reinterpret_cast<QAndroidAudioSinkStream *>(userData);
        Q_ASSERT(stream);
        stream->handleError(error);
    };

    builder.setupBuilder();
    m_stream = std::make_unique<QtAAudio::Stream>(builder);
}

bool QAndroidAudioSinkStream::open()
{
    if (!m_stream->isOpen()) {
        qCWarning(qLcAndroidAudioSink) << "Stream null";
        requestStop();
        return false;
    }

    if (!m_stream->areStreamParametersRespected())
        qCWarning(qLcAndroidAudioSink) << "Stream parameters not correct";

    return true;
}

bool QAndroidAudioSinkStream::start(QIODevice *device)
{
    Q_ASSERT(thread()->isCurrentThread());
    setQIODevice(device);
    pullFromQIODevice();
    createQIODeviceConnections(device);

    // TODO: Fill host ringbuffer before starting
    if (!m_stream->start()) {
        requestStop();
        return false;
    }

    return true;
}

QIODevice *QAndroidAudioSinkStream::start()
{
    auto *writer = createRingbufferWriterDevice();
    return start(writer) ? writer : nullptr;
}

bool QAndroidAudioSinkStream::start(AudioCallback cb)
{
    Q_ASSERT(thread()->isCurrentThread());
    m_audioCallback = std::move(cb);

    if (!m_stream->start()) {
        requestStop();
        return false;
    }

    return true;
}

void QAndroidAudioSinkStream::suspend()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->stop();
}

void QAndroidAudioSinkStream::resume()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->start();
}

void QAndroidAudioSinkStream::stop(ShutdownPolicy policy)
{
    requestStop();
    disconnectQIODeviceConnections();

    switch (policy) {
    case ShutdownPolicy::DrainRingbuffer:
        stop();
        break;
    case ShutdownPolicy::DiscardRingbuffer:
        reset();
        break;
    default:
        Q_UNREACHABLE_RETURN();
    }
}

void QAndroidAudioSinkStream::stop()
{
    if (isIdle() || m_audioCallback)
        return reset();

    stopIdleDetection();
    connectIdleHandler([this] {
        Q_ASSERT(thread()->isCurrentThread());
        if (!isIdle()) // Only handle <not idle> -> <idle> transitions
            return;

        // We have written everything we want to write, synchronous stop on application thread
        m_stream->stop();
        m_self = nullptr; // might delete the instance
    });

    m_parent = nullptr;

    // Take ownership of self to avoid deletion until AAudio stream is stopped
    m_self = shared_from_this();
}

void QAndroidAudioSinkStream::reset()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->stop();
}

void QAndroidAudioSinkStream::updateStreamIdle(bool arg)
{
    if (m_parent)
        m_parent->updateStreamIdle(arg);
}

QSpan<std::byte>
QAndroidAudioSinkStream::getHostSpan(void *audioData,
                                     int numFrames) const noexcept QT_MM_NONBLOCKING
{
    qsizetype byteAmount = m_nativeSampleFormat
            ? (QAudioHelperInternal::bytesPerSample(*m_nativeSampleFormat) * m_format.channelCount()
               * numFrames)
            : m_format.bytesForFrames(numFrames);
    return QSpan<std::byte>{ reinterpret_cast<std::byte *>(audioData), byteAmount };
}

aaudio_data_callback_result_t
QAndroidAudioSinkStream::processRingbuffer(void *audioData,
                                           int numFrames) noexcept QT_MM_NONBLOCKING
{
    auto consumedFrames = QPlatformAudioSinkStream::process(getHostSpan(audioData, numFrames),
                                                            numFrames, m_nativeSampleFormat);
    if (consumedFrames != static_cast<uint64_t>(numFrames) && isStopRequested())
        return AAUDIO_CALLBACK_RESULT_STOP;

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

aaudio_data_callback_result_t
QAndroidAudioSinkStream::processCallback(void *audioData, int numFrames) noexcept QT_MM_NONBLOCKING
{
    if (isStopRequested())
        return AAUDIO_CALLBACK_RESULT_STOP;

    QtMultimediaPrivate::runAudioCallback(*m_audioCallback, getHostSpan(audioData, numFrames),
                                          m_format, volume());
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void QAndroidAudioSinkStream::handleError(aaudio_result_t)
{
    // Handle as IO error which closes the stream
    // TODO: Check for underruns
    requestStop();
    invokeOnAppThread([this] {
        // clang-format off
        handleIOError(m_parent);
        // clang-format on
    });
}

QAndroidAudioSink::QAndroidAudioSink(QAudioDevice device, const QAudioFormat &fmt, QObject *parent)
    : BaseClass(std::move(device), fmt, parent)
{
}

} // namespace QtAAudio

QT_END_NAMESPACE

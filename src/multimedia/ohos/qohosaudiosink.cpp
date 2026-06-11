// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosaudiosink_p.h"

#include <QtMultimedia/private/qaudiohelpers_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtOHAudio {

Q_STATIC_LOGGING_CATEGORY(qLcOhosAudioSink, "qt.multimedia.ohos.audiosink")

QOhosAudioSinkStream::QOhosAudioSinkStream(QAudioDevice device, const QAudioFormat &format,
                                           std::optional<qsizetype> ringbufferSize,
                                           QOhosAudioSink *parent, float volume,
                                           std::optional<QtMultimediaPrivate::NativePeriodFrames> hardwareBufferFrames,
                                           AudioEndpointRole role)
    : QtMultimediaPrivate::QPlatformAudioSinkStream(std::move(device), format, ringbufferSize,
                                                    hardwareBufferFrames, volume),
      m_parent(parent),
      m_role(role)
{
    QAudioFormat builderFormat = format;
    const QAudioFormat::SampleFormat preferredFormat =
            QtOHAudio::preferredCompatibleSampleFormat(format.sampleFormat());
    if (preferredFormat != format.sampleFormat()) {
        m_hostFormat = format;
        builderFormat.setSampleFormat(preferredFormat);
    }

    QtOHAudio::StreamBuilder builder(builderFormat, AUDIOSTREAM_TYPE_RENDERER);

    qCDebug(qLcOhosAudioSink) << "Creating sink for device id:" << m_audioDevice.id()
                              << "description:" << m_audioDevice.description();

    switch (m_role) {
    case QtMultimediaPrivate::AudioEndpointRole::SoundEffect:
        builder.params.outputUsage = AUDIOSTREAM_USAGE_GAME;
        break;
    case QtMultimediaPrivate::AudioEndpointRole::Accessibility:
        builder.params.outputUsage = AUDIOSTREAM_USAGE_ACCESSIBILITY;
        break;
    case QtMultimediaPrivate::AudioEndpointRole::MediaPlayback:
    case QtMultimediaPrivate::AudioEndpointRole::Other:
        builder.params.outputUsage = AUDIOSTREAM_USAGE_MUSIC;
        break;
    }

    builder.userData = this;
    builder.writeCallback = [](OH_AudioRenderer *, void *userData, void *audioData,
                               int32_t audioDataSize) -> OH_AudioData_Callback_Result {
        auto *stream = reinterpret_cast<QOhosAudioSinkStream *>(userData);
        Q_ASSERT(stream);
        QSpan<std::byte> audioSpan = stream->getHostSpan(audioData, audioDataSize);
        return stream->m_audioCallback ? stream->processCallback(audioSpan)
                                       : stream->processRingbuffer(audioSpan, audioDataSize);
    };

    builder.setupBuilder();
    m_stream = std::make_unique<QtOHAudio::Stream>(builder);
}

bool QOhosAudioSinkStream::open()
{
    if (!m_stream || !m_stream->isOpen()) {
        qCWarning(qLcOhosAudioSink) << "Stream creation failed";
        requestStop();
        return false;
    }
    return true;
}

bool QOhosAudioSinkStream::start(QIODevice *device)
{
    Q_ASSERT(thread()->isCurrentThread());
    setQIODevice(device);
    pullFromQIODevice();
    createQIODeviceConnections(device);

    if (!m_stream->start()) {
        requestStop();
        return false;
    }
    return true;
}

QIODevice *QOhosAudioSinkStream::start()
{
    auto *writer = createRingbufferWriterDevice();
    return start(writer) ? writer : nullptr;
}

bool QOhosAudioSinkStream::start(AudioCallback cb)
{
    Q_ASSERT(thread()->isCurrentThread());
    m_audioCallback = std::move(cb);

    if (!m_stream->start()) {
        requestStop();
        return false;
    }
    return true;
}

void QOhosAudioSinkStream::suspend()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->pause();
}

void QOhosAudioSinkStream::resume()
{
    Q_ASSERT(thread()->isCurrentThread());
    m_stream->start();
}

void QOhosAudioSinkStream::stop(ShutdownPolicy policy)
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

void QOhosAudioSinkStream::stop()
{
    if (isIdle() || m_audioCallback)
        return reset();

    stopIdleDetection();
    connectIdleHandler([this] {
        Q_ASSERT(thread()->isCurrentThread());
        if (!isIdle())
            return;
        m_stream->stop();
        m_self = nullptr;
    });

    m_parent = nullptr;
    m_self = shared_from_this();
}

void QOhosAudioSinkStream::reset()
{
    // Note: reset() can be invoked on the engine's destruction thread when the
    // owning QRtAudioEngine was moved across threads after stream construction
    // (the stream's idle-detection notifier does not follow the sink across
    // moveToThread). OH_AudioRenderer_Stop is documented thread-safe, so we
    // intentionally do not assert thread()->isCurrentThread() here.
    m_stream->stop();
}

void QOhosAudioSinkStream::updateStreamIdle(bool arg)
{
    if (m_parent)
        m_parent->updateStreamIdle(arg);
}

QSpan<std::byte>
QOhosAudioSinkStream::getHostSpan(void *audioData,
                                  int32_t numBytes) const noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    return QSpan<std::byte>{ reinterpret_cast<std::byte *>(audioData),
                             static_cast<qsizetype>(numBytes) };
}

OH_AudioData_Callback_Result
QOhosAudioSinkStream::processRingbuffer(QSpan<std::byte> audioSpan,
                                        int32_t numBytes) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    const QAudioFormat &format = m_hostFormat.value_or(m_format);
    const int32_t numFrames = numBytes / format.bytesPerFrame();

    auto consumedFrames = m_hostFormat
            ? QPlatformAudioSinkStream::process(
                      audioSpan, numFrames,
                      QAudioHelperInternal::toNativeSampleFormat(m_hostFormat->sampleFormat()))
            : QPlatformAudioSinkStream::process(audioSpan, numFrames);

    if (consumedFrames != static_cast<uint64_t>(numFrames) && isStopRequested())
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;

    return AUDIO_DATA_CALLBACK_RESULT_VALID;
}

OH_AudioData_Callback_Result
QOhosAudioSinkStream::processCallback(QSpan<std::byte> audioSpan) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    if (isStopRequested())
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;

    if (m_hostFormat)
        QtMultimediaPrivate::runAudioCallback(*m_audioCallback, audioSpan, m_format, volume(),
                                              *m_hostFormat);
    else
        QtMultimediaPrivate::runAudioCallback(*m_audioCallback, audioSpan, m_format, volume());

    return AUDIO_DATA_CALLBACK_RESULT_VALID;
}

QOhosAudioSink::QOhosAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : BaseClass(std::move(device), format, parent)
{
}

QOhosAudioSink::~QOhosAudioSink() = default;

} // namespace QtOHAudio

QT_END_NAMESPACE

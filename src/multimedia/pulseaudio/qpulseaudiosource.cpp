// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpulseaudiosource_p.h"

#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qpulseaudio_contextmanager_p.h>
#include <QtMultimedia/private/qpulsehelpers_p.h>

#include <mutex> // for std::lock_guard
#include <unistd.h>

QT_BEGIN_NAMESPACE

namespace QPulseAudioInternal {

using namespace QtMultimediaPrivate;

QPulseAudioSourceStream::QPulseAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                                 std::optional<qsizetype> ringbufferSize,
                                                 QPulseAudioSource *parent,
                                                 float volume,
                                                 std::optional<int32_t> hardwareBufferSize)
    : QPlatformAudioSourceStream{
          std::move(device), format, ringbufferSize, hardwareBufferSize, volume,
      },
      m_parent(parent)
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    pa_sample_spec spec = QPulseAudioInternal::audioFormatToSampleSpec(format);
    pa_channel_map channel_map = QPulseAudioInternal::channelMapForAudioFormat(format);

    if (!pa_sample_spec_valid(&spec))
        return;

    const QByteArray streamName =
            QStringLiteral("QtmPulseStream-%1-%2").arg(::getpid()).arg(quintptr(this)).toUtf8();

    if (Q_UNLIKELY(qLcPulseAudioIn().isEnabled(QtDebugMsg))) {
        qCDebug(qLcPulseAudioIn) << "Format: " << spec.format;
        qCDebug(qLcPulseAudioIn) << "Rate: " << spec.rate;
        qCDebug(qLcPulseAudioIn) << "Channels: " << spec.channels;
        qCDebug(qLcPulseAudioIn) << "Frame size: " << pa_frame_size(&spec);
    }

    std::lock_guard engineLock{ *pulseEngine };

    m_stream = PAStreamHandle{
        pa_stream_new(pulseEngine->context(), streamName.constData(), &spec, &channel_map),
        PAStreamHandle::HasRef,
    };
}

QPulseAudioSourceStream::~QPulseAudioSourceStream() = default;

bool QPulseAudioSourceStream::start(QIODevice *device)
{
    setQIODevice(device);

    createQIODeviceConnections(device);

    return startStream(StreamType::Ringbuffer);
}

bool QPulseAudioSourceStream::start(AudioCallback &&audioCallback)
{
    m_audioCallback = std::move(audioCallback);
    return startStream(StreamType::Callback);
}

QIODevice *QPulseAudioSourceStream::start()
{
    QIODevice *device = createRingbufferReaderDevice();
    bool started = start(device);
    if (!started)
        return nullptr;

    return device;
}

void QPulseAudioSourceStream::stop(ShutdownPolicy shutdownPolicy)
{
    requestStop();

    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::lock_guard engineLock{ *pulseEngine };

    uninstallCallbacks();
    disconnectQIODeviceConnections();

    if (shutdownPolicy == ShutdownPolicy::DrainRingbuffer) {
        size_t bytesToRead = pa_stream_readable_size(m_stream.get());
        if (bytesToRead != size_t(-1))
            readCallbackRingbuffer(bytesToRead);
    }

    // Note: we need to cork the stream before disconnecting to prevent pulseaudio from deadlocking
    pa_stream_cork(m_stream.get(), 1, nullptr, nullptr);

    pa_stream_disconnect(m_stream.get());

    finalizeQIODevice(shutdownPolicy);
    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer)
        emptyRingbuffer();
}

void QPulseAudioSourceStream::suspend()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::lock_guard engineLock{ *pulseEngine };

    pa_stream_cork(m_stream.get(), 1, nullptr, nullptr);
}

void QPulseAudioSourceStream::resume()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::lock_guard engineLock{ *pulseEngine };

    pa_stream_cork(m_stream.get(), 0, nullptr, nullptr);
}

bool QPulseAudioSourceStream::open() const
{
    return bool(m_stream);
}

void QPulseAudioSourceStream::updateStreamIdle(bool idle)
{
    m_parent->updateStreamIdle(idle);
}

bool QPulseAudioSourceStream::startStream(StreamType streamType)
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    static const bool serverIsPipewire = [&] {
        return pulseEngine->serverName().contains(u"PulseAudio (on PipeWire");
    }();

    pa_buffer_attr attr{
        .maxlength = uint32_t(m_format.bytesForFrames(m_hardwareBufferFrames.value_or(1024))),
        .tlength = uint32_t(-1),
        .prebuf = uint32_t(-1),
        .minreq = uint32_t(-1),

        // pulseaudio's vanilla implementation requires us to set a fragment size, otherwise we only
        // get a single callback every 2-ish seconds.
        .fragsize = serverIsPipewire
                ? uint32_t(-1)
                : uint32_t(m_format.bytesForFrames(m_hardwareBufferFrames.value_or(1024))),
    };

    constexpr pa_stream_flags flags =
            pa_stream_flags(PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY);

    std::lock_guard engineLock{ *pulseEngine };
    installCallbacks(streamType);

    int status = pa_stream_connect_record(m_stream.get(), m_audioDevice.id().data(), &attr, flags);
    if (status != 0) {
        qCWarning(qLcPulseAudioOut) << "pa_stream_connect_record() failed!";
        m_stream = {};
        return false;
    }
    return true;
}

void QPulseAudioSourceStream::installCallbacks(StreamType streamType)
{
    pa_stream_set_overflow_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSourceStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->underflowCallback();
    }, this);

    pa_stream_set_underflow_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSourceStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->overflowCallback();
    }, this);

    pa_stream_set_state_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSourceStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->stateCallback();
    }, this);

    switch (streamType) {
    case StreamType::Ringbuffer: {
        pa_stream_set_read_callback(m_stream.get(),
                                    [](pa_stream *stream, size_t nbytes, void *data) {
            auto *self = reinterpret_cast<QPulseAudioSourceStream *>(data);
            Q_ASSERT(stream == self->m_stream.get());
            self->readCallbackRingbuffer(nbytes);
        }, this);
        break;
    }
    case StreamType::Callback: {
        pa_stream_set_read_callback(m_stream.get(),
                                    [](pa_stream *stream, size_t nbytes, void *data) {
            auto *self = reinterpret_cast<QPulseAudioSourceStream *>(data);
            Q_ASSERT(stream == self->m_stream.get());
            self->readCallbackAudioCallback(nbytes);
        }, this);
        break;
    }
    }

    pa_stream_set_latency_update_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSourceStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->latencyUpdateCallback();
    }, this);
}

void QPulseAudioSourceStream::uninstallCallbacks()
{
    pa_stream_set_overflow_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_underflow_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_state_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_read_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_latency_update_callback(m_stream.get(), nullptr, nullptr);
}

void QPulseAudioSourceStream::readCallbackRingbuffer([[maybe_unused]] size_t bytesToRead)
{
    const void *data{};
    size_t nBytes{};
    int status = pa_stream_peek(m_stream.get(), &data, &nBytes);
    if (status < 0) {
        invokeOnAppThread([this] {
            handleIOError(m_parent);
        });
        return;
    }

    QSpan<const std::byte> hostBuffer{
        reinterpret_cast<const std::byte *>(data),
        qsizetype(nBytes),
    };

    uint32_t numberOfFrames = m_format.framesForBytes(nBytes);

    [[maybe_unused]] uint64_t framesWritten =
            QPlatformAudioSourceStream::process(hostBuffer, numberOfFrames);
    status = pa_stream_drop(m_stream.get());
    if (status < 0) {
        if (!isStopRequested()) {
            invokeOnAppThread([this] {
                handleIOError(m_parent);
            });
        }
    }
}

void QPulseAudioSourceStream::readCallbackAudioCallback([[maybe_unused]] size_t bytesToRead)
{
    const void *data{};
    size_t nBytes{};
    int status = pa_stream_peek(m_stream.get(), &data, &nBytes);
    if (status < 0) {
        QMetaObject::invokeMethod(m_parent, [this] {
            handleIOError(m_parent);
        });
        return;
    }

    QSpan<const std::byte> hostBuffer{
        reinterpret_cast<const std::byte *>(data),
        qsizetype(nBytes),
    };

    runAudioCallback(*m_audioCallback, hostBuffer, m_format, volume());

    status = pa_stream_drop(m_stream.get());
    if (status < 0) {
        if (!isStopRequested()) {
            QMetaObject::invokeMethod(m_parent, [this] {
                handleIOError(m_parent);
            });
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QPulseAudioSource::QPulseAudioSource(QAudioDevice device, const QAudioFormat &format,
                                     QObject *parent)
    : BaseClass(std::move(device), format, parent)
{
}

bool QPulseAudioSource::validatePulseaudio()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    if (!pulseEngine->contextIsGood()) {
        qWarning() << "Invalid PulseAudio context:" << pulseEngine->getContextState();
        setError(QtAudio::Error::FatalError);
        return false;
    }
    return true;
}

void QPulseAudioSource::start(QIODevice *device)
{
    if (!validatePulseaudio())
        return;
    return BaseClass::start(device);
}

void QPulseAudioSource::start(AudioCallback &&cb)
{
    if (!validatePulseaudio())
        return;
    return BaseClass::start(std::move(cb));
}

QIODevice *QPulseAudioSource::start()
{
    if (!validatePulseaudio())
        return nullptr;
    return BaseClass::start();
}

} // namespace QPulseAudioInternal

QT_END_NAMESPACE

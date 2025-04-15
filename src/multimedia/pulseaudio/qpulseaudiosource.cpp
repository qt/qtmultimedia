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

struct QPulseAudioSourceStream final : QPlatformAudioSourceStream
{
    using SourceType = QPulseAudioSource;

    QPulseAudioSourceStream(QAudioDevice, const QAudioFormat &,
                            std::optional<qsizetype> ringbufferSize, QPulseAudioSource *parent,
                            float volume, std::optional<int32_t> hardwareBufferSize);
    Q_DISABLE_COPY_MOVE(QPulseAudioSourceStream)
    ~QPulseAudioSourceStream();

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::setVolume;

    bool start(QIODevice *device);
    QIODevice *start();
    void stop(ShutdownPolicy);
    void suspend();
    void resume();
    bool open() const;

    void updateStreamIdle(bool idle) override;

private:
    bool startStream();
    void installCallbacks();
    void uninstallCallbacks();

    QPulseAudioSource *m_parent;
    PAStreamHandle m_stream;

    // PulseAudio callbacks
    void underflowCallback() { }
    void overflowCallback() { }
    void stateCallback() { }
    void readCallback(size_t bytesToRead);
    void latencyUpdateCallback() { }
};

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

    std::unique_lock engineLock{ *pulseEngine };

    m_stream = PAStreamHandle{
        pa_stream_new(pulseEngine->context(), streamName.constData(), &spec, &channel_map),
        PAStreamHandle::HasRef,
    };

    installCallbacks();
}

QPulseAudioSourceStream::~QPulseAudioSourceStream() = default;

bool QPulseAudioSourceStream::start(QIODevice *device)
{
    setQIODevice(device);

    createQIODeviceConnections(device);

    bool streamStarted = startStream();
    return streamStarted;
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
    std::unique_lock engineLock{ *pulseEngine };

    uninstallCallbacks();
    disconnectQIODeviceConnections();

    if (shutdownPolicy == ShutdownPolicy::DrainRingbuffer) {
        size_t bytesToRead = pa_stream_readable_size(m_stream.get());
        if (bytesToRead != size_t(-1))
            readCallback(bytesToRead);
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
    std::unique_lock engineLock{ *pulseEngine };

    pa_stream_cork(m_stream.get(), 1, nullptr, nullptr);
}

void QPulseAudioSourceStream::resume()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::unique_lock engineLock{ *pulseEngine };

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

bool QPulseAudioSourceStream::startStream()
{
    pa_buffer_attr attr{
        .maxlength = uint32_t(m_format.bytesForFrames(m_hardwareBufferFrames.value_or(1024))),
        .tlength = uint32_t(-1),
        .prebuf = uint32_t(-1),
        .minreq = uint32_t(-1),
        .fragsize = uint32_t(-1),
    };

    constexpr pa_stream_flags flags =
            pa_stream_flags(PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY);

    int status = pa_stream_connect_record(m_stream.get(), m_audioDevice.id().data(), &attr, flags);
    if (status != 0) {
        qCWarning(qLcPulseAudioOut) << "pa_stream_connect_record() failed!";
        m_stream = {};
        return false;
    }
    return true;
}

void QPulseAudioSourceStream::installCallbacks()
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

    pa_stream_set_read_callback(m_stream.get(), [](pa_stream *stream, size_t nbytes, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSourceStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->readCallback(nbytes);
    }, this);

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

void QPulseAudioSourceStream::readCallback([[maybe_unused]] size_t bytesToRead)
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

    uint32_t numberOfFrames = m_format.framesForBytes(nBytes);

    [[maybe_unused]] uint64_t framesWritten =
            QPlatformAudioSourceStream::process(hostBuffer, numberOfFrames);
    status = pa_stream_drop(m_stream.get());
    if (status < 0) {
        if (!isStopRequested()) {
            QMetaObject::invokeMethod(m_parent, [this] {
                handleIOError(m_parent);
            });
        }
    }
}

} // namespace QPulseAudioInternal

QPulseAudioSource::QPulseAudioSource(QAudioDevice device, const QAudioFormat &format,
                                     QObject *parent)
    : QPlatformAudioSource(std::move(device), format, parent)
{
}

QPulseAudioSource::~QPulseAudioSource()
{
    stop();
}

template <typename Functor>
void QPulseAudioSource::startHelper(Functor &&starter)
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    if (!pulseEngine->contextIsGood()) {
        qWarning() << "Invalid PulseAudio context:" << pulseEngine->getContextState();
        setError(QtAudio::Error::FatalError);
        return;
    }

    m_stream = std::make_shared<QPulseAudioSourceStream>(m_audioDevice, format(), m_bufferSize,
                                                         this, volume(), m_hardwareBufferFrames);
    if (!m_stream->open()) {
        setError(QtAudio::Error::OpenError);
        m_stream = {};
        return;
    }

    bool started = starter(m_stream);
    if (started) {
        updateStreamState(QtAudio::State::ActiveState);
    } else {
        setError(QtAudio::Error::OpenError);
        m_stream = {};
    }
}

using SharedSourceStream = std::shared_ptr<QPulseAudioInternal::QPulseAudioSourceStream>;

void QPulseAudioSource::start(QIODevice *device)
{
    startHelper([&](const SharedSourceStream &stream) {
        return stream->start(device);
    });
}

QIODevice *QPulseAudioSource::start()
{
    QIODevice *deviceToReturn{};

    startHelper([&](const SharedSourceStream &stream) {
        deviceToReturn = stream->start();
        // HACK alert: we're "idle" until a consumer starts reading.
        // this is fundamentally broken, and we should fix this behavior
        updateStreamIdle(true, EmitStateSignal::False);
        return bool(deviceToReturn);
    });

    return deviceToReturn;
}

using ShutdownPolicy = QtMultimediaPrivate::QPlatformAudioIOStream::ShutdownPolicy;

void QPulseAudioSource::stop()
{
    if (!m_stream)
        return;

    if (m_stream->deviceIsRingbufferReader())
        // we own the qiodevice, so let's keep it alive to allow users to drain the ringbuffer
        m_retiredStream = m_stream;

    m_stream->stop(ShutdownPolicy::DrainRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QPulseAudioSource::reset()
{
    m_retiredStream = {};

    if (!m_stream)
        return;

    m_stream->stop(ShutdownPolicy::DiscardRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QPulseAudioSource::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    updateStreamState(QtAudio::State::SuspendedState);
}

void QPulseAudioSource::resume()
{
    if (!m_stream)
        return;

    if (state() == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    updateStreamState(QtAudio::State::ActiveState);
}

qsizetype QPulseAudioSource::bytesReady() const
{
    if (m_stream)
        return m_stream->bytesReady();
    return 0;
}

void QPulseAudioSource::setBufferSize(qsizetype value)
{
    if (value <= 0)
        m_bufferSize = {};
    else
        m_bufferSize = value;
}

qsizetype QPulseAudioSource::bufferSize() const
{
    return m_bufferSize.value_or(-1);
}

qint64 QPulseAudioSource::processedUSecs() const
{
    if (m_stream)
        return m_stream->processedDuration().count();
    return 0;
}

QT_END_NAMESPACE

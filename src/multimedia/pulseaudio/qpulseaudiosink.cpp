// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpulseaudiosink_p.h"

#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>

#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qpulseaudio_contextmanager_p.h>
#include <QtMultimedia/private/qpulsehelpers_p.h>

#include <mutex> // for std::lock_guard
#include <unistd.h>

QT_BEGIN_NAMESPACE

namespace QPulseAudioInternal {

using namespace QtMultimediaPrivate;

struct QPulseAudioSinkStream final : QPlatformAudioSinkStream
{
    using SinkType = QPulseAudioSink;

    QPulseAudioSinkStream(QAudioDevice, const QAudioFormat &format,
                          std::optional<qsizetype> ringbufferSize, QPulseAudioSink *parent,
                          float volume, std::optional<int32_t> hardwareBufferSize,
                          AudioEndpointRole);
    ~QPulseAudioSinkStream();

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::setVolume;

    bool start(QIODevice *device);
    QIODevice *start();
    void stop(ShutdownPolicy);
    void suspend();
    void resume();

    bool open() const;

private:
    void installCallbacks();
    void uninstallCallbacks();

    bool startStream();

    void updateStreamIdle(bool) override;

    // PulseAudio callbacks
    void underflowCallback() { }
    void overflowCallback() { }
    void stateCallback() { }
    void writeCallback(size_t requestedBytes);
    void latencyUpdateCallback() { }

    QPulseAudioSink *m_parent;
    PAStreamHandle m_stream;
};

QPulseAudioSinkStream::QPulseAudioSinkStream(QAudioDevice device, const QAudioFormat &format,
                                             std::optional<qsizetype> ringbufferSize, QPulseAudioSink *parent,
                                             float volume,
                                             std::optional<int32_t> hardwareBufferSize,
                                             AudioEndpointRole role)
    : QPlatformAudioSinkStream{
          std::move(device), format, ringbufferSize, hardwareBufferSize, volume,
      },
      m_parent{
          parent,
      }
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();

    pa_sample_spec spec = QPulseAudioInternal::audioFormatToSampleSpec(format);
    pa_channel_map channel_map = QPulseAudioInternal::channelMapForAudioFormat(format);

    if (Q_UNLIKELY(qLcPulseAudioOut().isEnabled(QtDebugMsg))) {
        qCDebug(qLcPulseAudioOut) << "Opening stream with.";
        qCDebug(qLcPulseAudioOut) << "\tFormat: " << spec.format;
        qCDebug(qLcPulseAudioOut) << "\tRate: " << spec.rate;
        qCDebug(qLcPulseAudioOut) << "\tChannels: " << spec.channels;
        qCDebug(qLcPulseAudioOut) << "\tFrame size: " << pa_frame_size(&spec);
    }

    const QByteArray streamName =
            QStringLiteral("QtmPulseStream-%1-%2").arg(::getpid()).arg(quintptr(this)).toUtf8();

    PAProplistHandle propList{
        pa_proplist_new(),
    };
    const char *roleString = [&]() -> const char * {
        switch (role) {
        case AudioEndpointRole::MediaPlayback:
            return "music";
        case AudioEndpointRole::SoundEffect:
            return "event";
        case AudioEndpointRole::Other:
            return nullptr;
        default:
            Q_UNREACHABLE_RETURN(nullptr);
        }
    }();

    if (roleString)
        pa_proplist_sets(propList.get(), PA_PROP_MEDIA_ROLE, roleString);

    std::unique_lock engineLock{ *pulseEngine };

    m_stream = PAStreamHandle{
        pa_stream_new_with_proplist(pulseEngine->context(), streamName.constData(), &spec,
                                    &channel_map, propList.get()),
        PAStreamHandle::HasRef,
    };

    if (!m_stream) {
        qWarning() << "Failed to create PulseAudio stream";
        return;
    }

    installCallbacks();
}

QPulseAudioSinkStream::~QPulseAudioSinkStream() = default;

bool QPulseAudioSinkStream::start(QIODevice *device)
{
    setQIODevice(device);
    pullFromQIODevice();

    createQIODeviceConnections(device);

    bool streamStarted = startStream();
    return streamStarted;
}

QIODevice *QPulseAudioSinkStream::start()
{
    QIODevice *device = createRingbufferReaderDevice();

    setIdleState(true);
    bool started = start(device);
    if (!started)
        return nullptr;

    return device;
}

void QPulseAudioSinkStream::stop(ShutdownPolicy policy)
{
    requestStop();

    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::unique_lock engineLock{ *pulseEngine };

    uninstallCallbacks();
    // Note: we need to cork to ensure that the stream is stopped immediately
    pa_stream_cork(m_stream.get(), 1, nullptr, nullptr);

    switch (policy) {
    case ShutdownPolicy::DrainRingbuffer: {
        bool writeFailed = false;

        visitRingbuffer([&](auto &ringbuffer) {
            ringbuffer.consumeAll([&](auto region) {
                if (writeFailed)
                    return;

                QSpan<const std::byte> writeRegion = as_bytes(region);
                int status = pa_stream_write(m_stream.get(), writeRegion.data(), writeRegion.size(),
                                             /*free_cb= */ nullptr, /*offset=*/0, PA_SEEK_RELATIVE);
                if (status != 0) {
                    handleIOError(m_parent);
                    writeFailed = true;
                }
            });
        });

        break;
    }
    case ShutdownPolicy::DiscardRingbuffer: {
        break;
    }
    default:
        Q_UNREACHABLE_RETURN();
    }
    pa_stream_disconnect(m_stream.get());
}

void QPulseAudioSinkStream::suspend()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::unique_lock engineLock{ *pulseEngine };

    pa_stream_cork(m_stream.get(), 1, nullptr, nullptr);
}

void QPulseAudioSinkStream::resume()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::unique_lock engineLock{ *pulseEngine };

    pa_stream_cork(m_stream.get(), 0, nullptr, nullptr);
}

bool QPulseAudioSinkStream::open() const
{
    return m_stream.isValid();
}

void QPulseAudioSinkStream::installCallbacks()
{
    pa_stream_set_overflow_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSinkStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->underflowCallback();
    }, this);

    pa_stream_set_underflow_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSinkStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->overflowCallback();
    }, this);

    pa_stream_set_state_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSinkStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->stateCallback();
    }, this);

    pa_stream_set_write_callback(m_stream.get(), [](pa_stream *stream, size_t nbytes, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSinkStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->writeCallback(nbytes);
    }, this);

    pa_stream_set_latency_update_callback(m_stream.get(), [](pa_stream *stream, void *data) {
        auto *self = reinterpret_cast<QPulseAudioSinkStream *>(data);
        Q_ASSERT(stream == self->m_stream.get());
        self->latencyUpdateCallback();
    }, this);
}

void QPulseAudioSinkStream::uninstallCallbacks()
{
    pa_stream_set_overflow_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_underflow_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_state_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_write_callback(m_stream.get(), nullptr, nullptr);
    pa_stream_set_latency_update_callback(m_stream.get(), nullptr, nullptr);
}

bool QPulseAudioSinkStream::startStream()
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

    int status = pa_stream_connect_playback(m_stream.get(), m_audioDevice.id().data(), &attr, flags,
                                            nullptr, nullptr);

    if (status != 0) {
        qCWarning(qLcPulseAudioOut) << "pa_stream_connect_playback() failed!";
        m_stream = {};
        return false;
    }
    return true;
}

void QPulseAudioSinkStream::updateStreamIdle(bool idle)
{
    m_parent->updateStreamIdle(idle);
}

void QPulseAudioSinkStream::writeCallback(size_t requestedBytes)
{
    // ensure round down to number of requested frames
    uint32_t requestedFrames = m_format.framesForBytes(requestedBytes);
    size_t nbytes = m_format.bytesForFrames(requestedFrames);

    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    Q_ASSERT(pulseEngine->isInMainLoop());

    void *dest = nullptr;

    int status = pa_stream_begin_write(m_stream.get(), &dest, &nbytes);
    if (status != 0) {
        qCWarning(qLcPulseAudioOut)
                << "pa_stream_begin_write error:" << currentError(pulseEngine->context());

        QMetaObject::invokeMethod(m_parent, [this] {
            handleIOError(m_parent);
        });
    }
    QSpan<std::byte> hostBuffer{ reinterpret_cast<std::byte *>(dest), qsizetype(nbytes) };

    const uint64_t consumedFrames = process(hostBuffer, requestedFrames);
    if (consumedFrames != requestedFrames) {
        auto remainder = drop(hostBuffer, m_format.bytesForFrames(consumedFrames));
        std::fill(remainder.begin(), remainder.end(), std::byte{});
    }
    status = pa_stream_write(m_stream.get(), hostBuffer.data(), nbytes,
                             /*free_cb= */ nullptr, /*offset=*/0, PA_SEEK_RELATIVE);
    if (status != 0) {
        qCWarning(qLcPulseAudioOut)
                << "pa_stream_begin_write error:" << currentError(pulseEngine->context());

        QMetaObject::invokeMethod(m_parent, [this] {
            handleIOError(m_parent);
        });
    }
}

} // namespace QPulseAudioInternal

QPulseAudioSink::QPulseAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : QPlatformAudioSink(std::move(device), format, parent)
{
}

QPulseAudioSink::~QPulseAudioSink()
{
    stop();
}

template <typename Functor>
void QPulseAudioSink::startHelper(Functor &&starter)
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    if (!pulseEngine->contextIsGood()) {
        qWarning() << "Invalid PulseAudio context:" << pulseEngine->getContextState();
        setError(QtAudio::Error::FatalError);
        return;
    }

    if (!m_format.isValid()) {
        qWarning() << "invalid format" << m_format;
        setError(QtAudio::Error::OpenError);
        return;
    }

    m_stream = std::make_shared<QPulseAudioSinkStream>(m_audioDevice, format(), m_bufferSize, this,
                                                       volume(), m_hardwareBufferFrames, m_role);
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

void QPulseAudioSink::start(QIODevice *device)
{
    startHelper([&](const std::shared_ptr<QPulseAudioSinkStream> &stream) {
        return stream->start(device);
    });
}

QIODevice *QPulseAudioSink::start()
{
    QIODevice *deviceToReturn{};

    startHelper([&](const std::shared_ptr<QPulseAudioSinkStream> &stream) {
        deviceToReturn = stream->start();
        updateStreamIdle(true, EmitStateSignal::False);
        return bool(deviceToReturn);
    });

    return deviceToReturn;
}

void QPulseAudioSink::stop()
{
    if (!m_stream)
        return;

    m_stream->stop(QPulseAudioSinkStream::ShutdownPolicy::DrainRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QPulseAudioSink::reset()
{
    if (!m_stream)
        return;

    m_stream->stop(QPulseAudioSinkStream::ShutdownPolicy::DiscardRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QPulseAudioSink::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    updateStreamState(QtAudio::State::SuspendedState);
}

void QPulseAudioSink::resume()
{
    if (!m_stream)
        return;

    if (state() == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    updateStreamState(QtAudio::State::ActiveState);
}

qsizetype QPulseAudioSink::bytesFree() const
{
    if (!m_stream)
        return 0;

    return m_stream->bytesFree();
}

void QPulseAudioSink::setBufferSize(qsizetype value)
{
    if (value <= 0)
        m_bufferSize = {};
    else
        m_bufferSize = value;
}

qsizetype QPulseAudioSink::bufferSize() const
{
    return m_bufferSize.value_or(-1);
}

qint64 QPulseAudioSink::processedUSecs() const
{
    if (m_stream)
        return m_stream->processedDuration().count();
    return 0;
}

void QPulseAudioSink::setVolume(float volume)
{
    QPlatformAudioEndpointBase::setVolume(volume);
    if (m_stream)
        m_stream->setVolume(volume);
}

QT_END_NAMESPACE

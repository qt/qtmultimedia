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
        case AudioEndpointRole::Accessibility:
            return "a11y";
        case AudioEndpointRole::Other:
            return nullptr;
        default:
            Q_UNREACHABLE_RETURN(nullptr);
        }
    }();

    if (roleString)
        pa_proplist_sets(propList.get(), PA_PROP_MEDIA_ROLE, roleString);

    std::lock_guard engineLock{ *pulseEngine };

    m_stream = PAStreamHandle{
        pa_stream_new_with_proplist(pulseEngine->context(), streamName.constData(), &spec,
                                    &channel_map, propList.get()),
        PAStreamHandle::HasRef,
    };

    if (!m_stream) {
        qWarning() << "Failed to create PulseAudio stream";
        return;
    }
}

QPulseAudioSinkStream::~QPulseAudioSinkStream() = default;

bool QPulseAudioSinkStream::start(QIODevice *device)
{
    setQIODevice(device);
    pullFromQIODevice();

    createQIODeviceConnections(device);

    bool streamStarted = startStream(StreamType::Ringbuffer);
    return streamStarted;
}

bool QPulseAudioSinkStream::start(AudioCallback &&callback)
{
    m_audioCallback = std::move(callback);

    bool streamStarted = startStream(StreamType::Callback);
    return streamStarted;
}

QIODevice *QPulseAudioSinkStream::start()
{
    QIODevice *device = createRingbufferWriterDevice();

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
    std::lock_guard engineLock{ *pulseEngine };

    uninstallCallbacks();
    // Note: we need to cork to ensure that the stream is stopped immediately
    pa_stream_cork(m_stream.get(), 1, nullptr, nullptr);

    if (m_audioCallback) {
        switch (policy) {
        case ShutdownPolicy::DrainRingbuffer:
        case ShutdownPolicy::DiscardRingbuffer:
            break;
        default:
            Q_UNREACHABLE_RETURN();
        }
    } else {
        switch (policy) {
        case ShutdownPolicy::DrainRingbuffer: {
            bool writeFailed = false;

            visitRingbuffer([&](auto &ringbuffer) {
                ringbuffer.consumeAll([&](auto region) {
                    if (writeFailed)
                        return;

                    QSpan<const std::byte> writeRegion = as_bytes(region);
                    int status =
                            pa_stream_write(m_stream.get(), writeRegion.data(), writeRegion.size(),
                                            /*free_cb= */ nullptr, /*offset=*/0, PA_SEEK_RELATIVE);
                    if (status != 0)
                        writeFailed = true;
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
    }
    pa_stream_disconnect(m_stream.get());
}

void QPulseAudioSinkStream::suspend()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::lock_guard engineLock{ *pulseEngine };

    pa_stream_cork(m_stream.get(), 1, nullptr, nullptr);
}

void QPulseAudioSinkStream::resume()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::lock_guard engineLock{ *pulseEngine };

    pa_stream_cork(m_stream.get(), 0, nullptr, nullptr);
}

bool QPulseAudioSinkStream::open() const
{
    return m_stream.isValid();
}

void QPulseAudioSinkStream::installCallbacks(StreamType streamType)
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

    switch (streamType) {
    case StreamType::Ringbuffer:
        pa_stream_set_write_callback(m_stream.get(),
                                     [](pa_stream *stream, size_t nbytes, void *data) {
            auto *self = reinterpret_cast<QPulseAudioSinkStream *>(data);
            Q_ASSERT(stream == self->m_stream.get());
            self->writeCallbackRingbuffer(nbytes);
        }, this);
        break;
    case StreamType::Callback:
        pa_stream_set_write_callback(m_stream.get(),
                                     [](pa_stream *stream, size_t nbytes, void *data) {
            auto *self = reinterpret_cast<QPulseAudioSinkStream *>(data);
            Q_ASSERT(stream == self->m_stream.get());
            self->writeCallbackAudioCallback(nbytes);
        }, this);
        break;

    default:
        Q_UNREACHABLE_RETURN();
    }

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

bool QPulseAudioSinkStream::startStream(StreamType streamType)
{
    pa_buffer_attr attr{
        .maxlength = uint32_t(m_format.bytesForFrames(m_hardwareBufferFrames.value_or(1024))),
        .tlength = uint32_t(-1),
        .prebuf = uint32_t(-1),
        .minreq = uint32_t(-1),
        .fragsize = uint32_t(-1),
    };

    installCallbacks(streamType);

    constexpr pa_stream_flags flags =
            pa_stream_flags(PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY);

    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    std::lock_guard engineLock{ *pulseEngine };

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

void QPulseAudioSinkStream::writeCallbackRingbuffer(size_t requestedBytes)
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

void QPulseAudioSinkStream::writeCallbackAudioCallback(size_t requestedBytes)
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

        invokeOnAppThread([this] {
            handleIOError(m_parent);
        });
    }
    QSpan<std::byte> hostBuffer{ reinterpret_cast<std::byte *>(dest), qsizetype(nbytes) };
    runAudioCallback(*m_audioCallback, hostBuffer, m_format, volume());

    status = pa_stream_write(m_stream.get(), hostBuffer.data(), nbytes,
                             /*free_cb= */ nullptr, /*offset=*/0, PA_SEEK_RELATIVE);
    if (status != 0) {
        qCWarning(qLcPulseAudioOut)
                << "pa_stream_begin_write error:" << currentError(pulseEngine->context());

        invokeOnAppThread([this] {
            handleIOError(m_parent);
        });
    }
}

QPulseAudioSink::QPulseAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : BaseClass(std::move(device), format, parent)
{
}

bool QPulseAudioSink::validatePulseaudio()
{
    QPulseAudioContextManager *pulseEngine = QPulseAudioContextManager::instance();
    if (!pulseEngine->contextIsGood()) {
        qWarning() << "Invalid PulseAudio context:" << pulseEngine->getContextState();
        setError(QtAudio::Error::FatalError);
        return false;
    }
    return true;
}

void QPulseAudioSink::start(QIODevice *device)
{
    if (!validatePulseaudio())
        return;
    return BaseClass::start(device);
}

void QPulseAudioSink::start(AudioCallback &&callback)
{
    if (!validatePulseaudio())
        return;
    return BaseClass::start(std::forward<AudioCallback>(callback));
}

QIODevice *QPulseAudioSink::start()
{
    if (!validatePulseaudio())
        return nullptr;
    return BaseClass::start();
}

} // namespace QPulseAudioInternal

QT_END_NAMESPACE

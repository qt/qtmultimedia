// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiosink_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_audiostream_p.h"
#include "qpipewire_support_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpointer.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qspan.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qautoresetevent_p.h>

#include <pipewire/pipewire.h>
#include <pipewire/stream.h>
#include <spa/pod/builder.h>

#include <thread>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_STATIC_LOGGING_CATEGORY(lcPipewireAudioSink, "qt.multimedia.pipewire.audiosink");
static constexpr bool pipewireRealtimeTracing = false;

using QtMultimediaPrivate::QPlatformAudioSinkStream;
using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;
using namespace Qt::Literals;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QPipewireAudioSinkStream

struct QPipewireAudioSinkStream final : std::enable_shared_from_this<QPipewireAudioSinkStream>,
                                        QPipewireAudioStream,
                                        QPlatformAudioSinkStream
{
    using SampleFormat = QAudioFormat::SampleFormat;

    QPipewireAudioSinkStream(QAudioDevice, QPipewireAudioSink *parent, const QAudioFormat &format,
                             AudioEndpointRole, std::optional<qsizetype> ringbufferSize,
                             std::optional<int32_t> hardwareBufferSize, float volume);
    ~QPipewireAudioSinkStream();

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::inferRingbufferBytes;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::ringbufferSizeInBytes;
    using QPlatformAudioSinkStream::setVolume;

    bool start(QIODevice *device);
    QIODevice *start();
    bool start(AudioCallback);

    void stop(ShutdownPolicy);

    void updateStreamIdle(bool idle) override;

private:
    void createStream(QPipewireAudioStream::StreamType);
    std::optional<ObjectSerial> findSinkNodeSerial();

    using QPlatformAudioSinkStream::m_format;

    // QPipewireAudioStream overrides
    void handleDeviceRemoved() override;
    void processRingbuffer() noexcept QT_MM_NONBLOCKING override;
    void processCallback() noexcept QT_MM_NONBLOCKING override;
    template <typename Functor>
    void processHelper(Functor &&f);

    void stateChanged(pw_stream_state /*old*/, pw_stream_state state,
                      const char * /*error*/) override;

    void disconnectStream();
    QSemaphore m_disconnectSemaphore;

    std::shared_ptr<QPipewireAudioSinkStream> m_self;

    std::atomic<ShutdownPolicy> m_shutdownPolicy{ ShutdownPolicy::DiscardRingbuffer };
    QAutoResetEvent m_ringbufferDrained;

    // process helpers
    void queueBuffer(struct pw_buffer *b, uint64_t samplesWritten) noexcept QT_MM_NONBLOCKING;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    [[maybe_unused]] static void fakeXRun();

    AudioEndpointRole m_role;
    std::optional<AudioCallback> m_audioCallback;

    QPipewireAudioSink *m_parent;
};

QPipewireAudioSinkStream::QPipewireAudioSinkStream(QAudioDevice device,
                                                   QPipewireAudioSink *parent,
                                                   const QAudioFormat &format, AudioEndpointRole role,
                                                   std::optional<qsizetype> ringbufferSize,
                                                   std::optional<int32_t> hardwareBufferFrames,
                                                   float volume):
    QPipewireAudioStream {
        format,
    },
    QPlatformAudioSinkStream {
        std::move(device),
        format,
        ringbufferSize,
        hardwareBufferFrames,
        volume,
    },
    m_role {
        role,
    },
    m_parent{
        parent,
    }
{
    m_xrunNotification = QObject::connect(&m_xrunOccurred, &QAutoResetEvent::activated,
                                          &m_xrunOccurred, [this, parent] {
        if (isStopRequested())
            return;
        parent->reportXRuns(m_xrunCount.exchange(0));
    });
}

QPipewireAudioSinkStream::~QPipewireAudioSinkStream()
{
    Q_ASSERT(!m_deviceRemovalObserver);
}

bool QPipewireAudioSinkStream::start(QIODevice *device)
{
    createStream(StreamType::Ringbuffer);

    Q_ASSERT(hasStream());
    auto sinkNodeSerial = findSinkNodeSerial();
    if (!sinkNodeSerial)
        return false;

    setQIODevice(device);
    pullFromQIODevice();

    createQIODeviceConnections(device);

    bool connected = connectStream(*sinkNodeSerial, SPA_DIRECTION_OUTPUT);
    if (!connected)
        return false;

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();

    return true;
}

QIODevice *QPipewireAudioSinkStream::start()
{
    QIODevice *device = createRingbufferReaderDevice();

    setIdleState(true);
    bool started = start(device);
    if (!started)
        return nullptr;

    return device;
}

bool QPipewireAudioSinkStream::start(AudioCallback audioCallback)
{
    createStream(StreamType::Callback);

    Q_ASSERT(hasStream());
    auto sinkNodeSerial = findSinkNodeSerial();
    if (!sinkNodeSerial)
        return false;

    m_audioCallback = std::move(audioCallback);

    bool connected = connectStream(*sinkNodeSerial, SPA_DIRECTION_OUTPUT);
    if (!connected)
        return false;

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();
    return true;
}

void QPipewireAudioSinkStream::stop(ShutdownPolicy shutdownPolicy)
{
    m_shutdownPolicy.store(shutdownPolicy, std::memory_order_relaxed);
    if (shutdownPolicy == ShutdownPolicy::DrainRingbuffer) {
        // disconnect when ringbuffer is drained
        QObject::connect(&m_ringbufferDrained, &QAutoResetEvent::activated, &m_ringbufferDrained,
                         [this] {
            disconnectStream();
        });
    }

    requestStop();
    m_parent = nullptr;

    disconnectQIODeviceConnections();

    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer || m_audioCallback) {
        // disconnect immediately
        disconnectStream();
    }

    unregisterDeviceObserver();

    if (m_audioCallback)
        // ensure that no callback is sent after we stop the stream
        m_disconnectSemaphore.acquire();
}

void QPipewireAudioSinkStream::updateStreamIdle(bool idle)
{
    m_parent->updateStreamIdle(idle);
}

void QPipewireAudioSinkStream::createStream(StreamType streamType)
{
    const char *roleString = [&] {
        switch (m_role) {
        case AudioEndpointRole::MediaPlayback:
        case AudioEndpointRole::Other:
            return "Music";
        case AudioEndpointRole::SoundEffect:
            return "Notification";
        default:
            Q_UNREACHABLE_RETURN("Music");
        }
    }();

    auto extraProperties = std::array{
        spa_dict_item{ PW_KEY_MEDIA_CATEGORY, "Playback" },
        spa_dict_item{ PW_KEY_MEDIA_ROLE, roleString },
    };

    QString applicationName = qApp->applicationName();
    if (applicationName.isNull())
        applicationName = u"QPipewireAudioSink"_s;

    QPipewireAudioStream::createStream(extraProperties, m_hardwareBufferFrames,
                                       applicationName.toUtf8().constData(), streamType);
}

std::optional<ObjectSerial> QPipewireAudioSinkStream::findSinkNodeSerial()
{
    const QPipewireAudioDevicePrivate *device =
            QAudioDevicePrivate::handle<QPipewireAudioDevicePrivate>(m_audioDevice);

    QByteArray nodeName = device->nodeName();
    auto ret = QAudioContextManager::deviceMonitor().findSinkNodeSerial(std::string_view{
            nodeName.data(),
            size_t(nodeName.size()),
    });

    if (!ret)
        qWarning() << "Cannot find device: " << nodeName;
    return ret;
}

void QPipewireAudioSinkStream::handleDeviceRemoved()
{
    if (!isStopRequested())
        // note: as long as the stream is not stopped, m_parent is valid
        handleIOError(m_parent);
}

static auto resolveHostBuffer(pw_buffer *b, const QAudioFormat &format)
{
    struct spa_buffer *buf = b->buffer;
    uint64_t strideBytes = format.bytesPerSample() * format.channelCount();
    Q_ASSERT(strideBytes > 0);
    uint64_t totalNumberOfFrames = buf->datas[0].maxsize / strideBytes;

#if PW_CHECK_VERSION(0, 3, 49)
    if (pw_check_library_version(0, 3, 49))
        // LATER: drop support for 0.3.49
        if (b->requested)
            totalNumberOfFrames = std::min(b->requested, totalNumberOfFrames);
#endif

    const uint64_t requestedSamples = totalNumberOfFrames * format.channelCount();

    QSpan<std::byte> writeBuffer{
        reinterpret_cast<std::byte *>(buf->datas[0].data),
        qsizetype(requestedSamples * format.bytesPerSample()),
    };

    struct HostBufferData
    {
        QSpan<std::byte> writeBuffer;
        const uint64_t requestedSamples{};
        const uint64_t totalNumberOfFrames{};
    };

    return HostBufferData{
        writeBuffer,
        requestedSamples,
        totalNumberOfFrames,
    };
}

void QPipewireAudioSinkStream::processRingbuffer() noexcept QT_MM_NONBLOCKING
{
    struct pw_buffer *b = pw_stream_dequeue_buffer(m_stream.get());
    if (!b) {
        qCritical() << "pw_stream_dequeue_buffer failed";
        return;
    }

    auto [writeBuffer, requestedSamples, totalNumberOfFrames] = resolveHostBuffer(b, m_format);

    bool stopRequested = isStopRequested(std::memory_order_acquire);
    ShutdownPolicy shutdownPolicy = stopRequested ? m_shutdownPolicy.load(std::memory_order_relaxed)
                                                  : ShutdownPolicy::DrainRingbuffer;

    if (stopRequested && shutdownPolicy == ShutdownPolicy::DiscardRingbuffer) {
        // discarding ringbuffer: we silence the last block and exit early
        ::memset(writeBuffer.data(), 0, writeBuffer.size());
        queueBuffer(b, requestedSamples);

        if constexpr (pipewireRealtimeTracing)
            qCDebug(lcPipewireAudioSink)
                    << "QPipewireAudioSinkStream: shutdown with DiscardRingbuffer";
        return;
    }

    performXRunDetection(totalNumberOfFrames);

    uint64_t framesWritten = QPlatformAudioSinkStream::process(writeBuffer, totalNumberOfFrames);
    uint64_t samplesWritten = framesWritten * m_format.channelCount();

    if (stopRequested) {
        if (samplesWritten < requestedSamples) {
            if constexpr (pipewireRealtimeTracing)
                qCDebug(lcPipewireAudioSink)
                        << "QPipewireAudioSinkStream: shutdown after draining ringbuffer";
            m_ringbufferDrained.set();
        }
    }

    queueBuffer(b, samplesWritten);
    addFramesHandled(samplesWritten);
}

void QPipewireAudioSinkStream::processCallback() noexcept QT_MM_NONBLOCKING
{
    using namespace QtMultimediaPrivate;
    struct pw_buffer *b = pw_stream_dequeue_buffer(m_stream.get());
    if (!b) {
        qCritical() << "pw_stream_dequeue_buffer failed";
        return;
    }

    auto [writeBuffer, requestedSamples, totalNumberOfFrames] = resolveHostBuffer(b, m_format);

    runAudioSinkCallback(*m_audioCallback, writeBuffer.data(), requestedSamples, m_format);

    queueBuffer(b, requestedSamples);
}

void QPipewireAudioSinkStream::stateChanged(pw_stream_state oldState, pw_stream_state state,
                                            const char *)
{
    qCDebug(lcPipewireAudioSink) << "QPipewireAudioSinkStream::stateChanged" << oldState << state;

    switch (state) {
    case pw_stream_state::PW_STREAM_STATE_UNCONNECTED: {
        m_disconnectSemaphore.release();
        m_self.reset();
        // CAVEAT: m_self may have been the last owner causing the object to be destroyed now.
        break;

    default:
        break;
    }
    }
}

void QPipewireAudioSinkStream::disconnectStream()
{
    auto self = shared_from_this(); // extend lifetime until this function returns;

    QPipewireAudioStream::disconnectStream();

    QObject::disconnect(m_xrunNotification);
}

void QPipewireAudioSinkStream::queueBuffer(pw_buffer *b, uint64_t samplesWritten) noexcept QT_MM_NONBLOCKING
{
    struct spa_buffer *buf = b->buffer;
    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = m_format.bytesPerSample() * m_format.channelCount();
    buf->datas[0].chunk->size = samplesWritten * m_format.bytesPerSample();

    pw_stream_queue_buffer(m_stream.get(), b);
}

void QPipewireAudioSinkStream::fakeXRun()
{
    constexpr bool forceXRun = true;
    if constexpr (forceXRun) {
        // force xrun
        static int i = 0;
        if (++i == 10)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QPipewireAudioSink

QPipewireAudioSink::QPipewireAudioSink(QAudioDevice device, const QAudioFormat &format,
                                       QObject *parent)
    : BaseClass(std::move(device), format, parent)
{
}

QPipewireAudioSink::~QPipewireAudioSink()
{
    stop();
}

template <typename Functor>
void QPipewireAudioSink::startHelper(Functor &&starter)
{
    m_stream = std::make_shared<QPipewireAudioSinkStream>(
            m_audioDevice, this, format(), m_role, m_bufferSize, m_hardwareBufferFrames, m_volume);

    bool started = starter(m_stream);
    if (started) {
        updateStreamState(QtAudio::State::ActiveState);
    } else {
        m_stream = {};
        setError(QtAudio::Error::OpenError);
    }
}

using SharedSinkStream = std::shared_ptr<QPipewireAudioSinkStream>;

void QPipewireAudioSink::start(QIODevice *device)
{
    startHelper([&](const SharedSinkStream &stream) {
        return stream->start(device);
    });
}

QIODevice *QPipewireAudioSink::start()
{
    QIODevice *deviceToReturn{};

    startHelper([&](const SharedSinkStream &stream) {
        deviceToReturn = stream->start();
        updateStreamIdle(true, EmitStateSignal::False);
        return bool(deviceToReturn);
    });

    return deviceToReturn;
}

void QPipewireAudioSink::stop()
{
    if (!m_stream)
        return;

    m_stream->stop(QPipewireAudioSinkStream::ShutdownPolicy::DrainRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QPipewireAudioSink::reset()
{
    if (!m_stream)
        return;

    m_stream->stop(QPipewireAudioSinkStream::ShutdownPolicy::DiscardRingbuffer);
    m_stream = {};
    updateStreamState(QtAudio::State::StoppedState);
}

void QPipewireAudioSink::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    updateStreamState(QtAudio::State::SuspendedState);
}

void QPipewireAudioSink::resume()
{
    if (!m_stream)
        return;

    if (state() == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    updateStreamState(QtAudio::State::ActiveState);
}

qsizetype QPipewireAudioSink::bytesFree() const
{
    if (!m_stream)
        return 0;

    return m_stream->bytesFree();
}

void QPipewireAudioSink::start(AudioCallback &&audioCallback)
{
    using namespace QtMultimediaPrivate;
    if (!validateAudioSinkCallback(audioCallback, m_format)) {
        setError(QAudio::OpenError);
        return;
    }

    startHelper([&](const SharedSinkStream &stream) {
        return stream->start(std::move(audioCallback));
    });
}

bool QPipewireAudioSink::hasCallbackAPI()
{
    return true;
}

void QPipewireAudioSink::reportXRuns(int numberOfXruns)
{
    qDebug() << "XRuns occurred:" << numberOfXruns;
}

void QPipewireAudioSink::setRole(AudioEndpointRole role)
{
    m_role = role;
}

} // namespace QtPipeWire

QT_END_NAMESPACE

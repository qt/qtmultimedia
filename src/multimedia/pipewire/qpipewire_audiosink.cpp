// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiosink_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_audiostream_p.h"
#include "qpipewire_support_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpointer.h>
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
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::setVolume;

    bool start(QIODevice *device);
    QIODevice *start();
    void stop(ShutdownPolicy);

    void updateStreamIdle(bool idle) override;

private:
    std::optional<ObjectSerial> findSinkNodeSerial();

    using QPlatformAudioSinkStream::m_format;

    // QPipewireAudioStream overrides
    void handleDeviceRemoved() override;
    void process() QT_MM_NONBLOCKING override;
    void stateChanged(pw_stream_state /*old*/, pw_stream_state state,
                      const char * /*error*/) override;

    void disconnectStream();

    std::shared_ptr<QPipewireAudioSinkStream> m_self;

    std::atomic<ShutdownPolicy> m_shutdownPolicy{ ShutdownPolicy::DiscardRingbuffer };
    QAutoResetEvent m_ringbufferDrained;

    // process helpers
    void queueBuffer(struct pw_buffer *b, uint64_t samplesWritten) QT_MM_NONBLOCKING;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    [[maybe_unused]] static void fakeXRun();

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
    m_parent{
        parent,
    }
{
    const char *roleString = [&] {
        switch (role) {
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

    createStream(extraProperties, hardwareBufferFrames, "QPipewireAudioSink");

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

    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer) {
        // disconnect immediately
        disconnectStream();
    }

    unregisterDeviceObserver();
}

void QPipewireAudioSinkStream::updateStreamIdle(bool idle)
{
    m_parent->updateStreamIdle(idle);
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
    if (!isStopRequested()) {
        // note: as long as the stream is not stopped, m_parent is valid
        m_parent->setError(QAudio::Error::IOError);
        m_parent->updateStreamState(QAudio::State::StoppedState);
    }
}

void QPipewireAudioSinkStream::process() QT_MM_NONBLOCKING
{
    struct pw_buffer *b = pw_stream_dequeue_buffer(m_stream.get());
    if (!b) {
        qCritical() << "pw_stream_dequeue_buffer failed";
        return;
    }

    struct spa_buffer *buf = b->buffer;
    uint64_t strideBytes = m_format.bytesPerSample() * m_format.channelCount();
    Q_ASSERT(strideBytes > 0);
    uint64_t totalNumberOfFrames = buf->datas[0].maxsize / strideBytes;

#if PW_CHECK_VERSION(0, 3, 49)
    if (pw_check_library_version(0, 3, 49))
        // LATER: drop support for 0.3.49
        if (b->requested)
            totalNumberOfFrames = std::min(b->requested, totalNumberOfFrames);
#endif

    const uint64_t requestedSamples = totalNumberOfFrames * m_format.channelCount();

    QSpan<std::byte> writeBuffer{
        reinterpret_cast<std::byte *>(buf->datas[0].data),
        qsizetype(requestedSamples * m_format.bytesPerSample()),
    };

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

void QPipewireAudioSinkStream::stateChanged(pw_stream_state oldState, pw_stream_state state,
                                            const char *)
{
    qCDebug(lcPipewireAudioSink) << "QPipewireAudioSinkStream::stateChanged" << oldState << state;

    switch (state) {
    case pw_stream_state::PW_STREAM_STATE_UNCONNECTED: {
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

void QPipewireAudioSinkStream::queueBuffer(pw_buffer *b, uint64_t samplesWritten) QT_MM_NONBLOCKING
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
    if (!m_format.isValid()) {
        qWarning() << "invalid format" << m_format;
        setError(QtAudio::Error::OpenError);
        return;
    }

    m_stream = std::make_shared<QPipewireAudioSinkStream>(
            m_audioDevice, this, format(), m_role, m_bufferSize, m_hardwareBufferFrames, m_volume);
    if (!m_stream->hasStream()) {
        setError(QtAudio::Error::OpenError);
        return;
    }

    bool started = starter(m_stream);
    if (started)
        updateStreamState(QtAudio::State::ActiveState);
    else
        setError(QtAudio::Error::OpenError);
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

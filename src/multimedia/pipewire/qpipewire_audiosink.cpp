// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiosink_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_audiostream_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpointer.h>
#include <QtCore/qspan.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include <QtMultimedia/private/qaudio_qiodevice_support_p.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>
#include <QtMultimedia/private/qautoresetevent_p.h>

#include <pipewire/pipewire.h>
#include <pipewire/stream.h>
#include <spa/pod/builder.h>

#include <thread>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_STATIC_LOGGING_CATEGORY(lcPipewireAudioSink, "qt.multimedia.pipewire.audiosink");
static constexpr bool pipewireRealtimeTracing = false;

using QtPrivate::QAudioRingBuffer;
using QtPrivate::QAutoResetEvent;
using QtPrivate::QIODeviceRingBufferWriter;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QPipewireAudioSinkStream

enum class ShutdownPolicy : uint8_t
{
    DrainRingbuffer,
    DiscardRingbuffer,
};

struct QPipewireAudioSinkStream final : std::enable_shared_from_this<QPipewireAudioSinkStream>,
                                        QPipewireAudioStream
{
    using SampleFormat = QAudioFormat::SampleFormat;

    QPipewireAudioSinkStream(QPipewireAudioSink *parent, const QAudioFormat &format,
                             std::optional<qsizetype> ringbufferSize,
                             std::optional<qsizetype> hardwareBufferSize = std::nullopt);

    ~QPipewireAudioSinkStream();

    qsizetype bytesFree() const;
    bool start(QIODevice *device, ObjectSerial sinkNodeSerial);
    QIODevice *start(ObjectSerial sinkNodeSerial);
    void stop(ShutdownPolicy);

    void setVolume(qreal);

    std::chrono::microseconds processedDuration();

private:
    // QPipewireAudioStream overrides
    void handleDeviceRemoved() override;
    void process() QT_MM_NONBLOCKING override;
    void stateChanged(pw_stream_state /*old*/, pw_stream_state state,
                      const char * /*error*/) override;

    void prepareFormat(const QAudioFormat &format, std::optional<qsizetype> ringbufferSize);

    void pullFromQIODevice();
    void disconnectStream();

    std::atomic<qreal> m_volume{ 1.f };

    using Ringbuffer = std::variant<QAudioRingBuffer<float>, QAudioRingBuffer<int32_t>,
                                    QAudioRingBuffer<int16_t>, QAudioRingBuffer<uint8_t>>;

    Ringbuffer m_ringbuffer{
        std::in_place_type_t<QAudioRingBuffer<float>>{},
        0,
    };

    uint32_t m_strideBytes{};

    QPointer<QIODevice> m_device;
    std::unique_ptr<QIODevice> m_ringbufferAdapter; // for push mode

    QAutoResetEvent m_ringbufferHasSpaceEvent;
    QMetaObject::Connection m_ringbufferHasSpaceConnection;
    QMetaObject::Connection m_iodeviceHasNewDataConnection;

    // LATER: do we want to relax notifying the app thread?
    static constexpr int notificationThreshold = 0;

    std::shared_ptr<QPipewireAudioSinkStream> m_self;

    std::atomic<ShutdownPolicy> m_shutdownPolicy{ ShutdownPolicy::DiscardRingbuffer };
    std::atomic_bool m_stopRequested{ false };
    QAutoResetEvent m_ringbufferDrained;

    // process helpers
    void queueBuffer(struct pw_buffer *b, uint64_t samplesWritten) QT_MM_NONBLOCKING;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    [[maybe_unused]] static void fakeXRun();

    // "idle state" detection
    void performIdleDetection(uint64_t samplesWritten, uint64_t sampleCount);
    QAutoResetEvent m_streamIdleDetectionNotifier;
    QMetaObject::Connection m_streamIdleDetectionConnection;
    std::atomic_bool m_streamIsIdle{ false };
    std::atomic<uint64_t> m_totalNumberOfSamplesConsumedFromRingbuffer{};

    QPipewireAudioSink *m_parent;
};

QPipewireAudioSinkStream::QPipewireAudioSinkStream(QPipewireAudioSink *parent,
                                                   const QAudioFormat &format,
                                                   std::optional<qsizetype> ringbufferSize,
                                                   std::optional<qsizetype> hardwareBufferSize):
    QPipewireAudioStream {
        format,
    },
    m_parent{
        parent,
    }
{
    prepareFormat(format, ringbufferSize);

    auto extraProperties = std::array{
        spa_dict_item{ PW_KEY_MEDIA_CATEGORY, "Playback" },
        spa_dict_item{ PW_KEY_MEDIA_ROLE, "Music" },
    };

    createStream(extraProperties, hardwareBufferSize, "QPipewireAudioSink");

    m_xrunNotification = QObject::connect(&m_xrunOccurred, &QAutoResetEvent::activated,
                                          &m_xrunOccurred, [this, parent] {
        if (m_stopRequested)
            return;
        parent->reportXRuns(m_xrunCount.exchange(0));
    });

    m_streamIdleDetectionConnection =
            QObject::connect(&m_streamIdleDetectionNotifier, &QAutoResetEvent::activated,
                             &m_streamIdleDetectionNotifier, [this, parent] {
        if (m_stopRequested)
            return;

        bool sinkIsIdle = m_streamIsIdle.load();

        if (sinkIsIdle) {
            // data has been pushed to the ringbuffer, while the stream is still idle, this will
            // change during the next audio callback
            bool ringbufferIsEmpty = std::visit([&](auto &ringbuffer) {
                return ringbuffer.free() == ringbuffer.size();
            }, m_ringbuffer);

            sinkIsIdle = ringbufferIsEmpty;
        }

        parent->streamIdle(sinkIsIdle);
    });
}

QPipewireAudioSinkStream::~QPipewireAudioSinkStream()
{
    Q_ASSERT(m_stopRequested);
    Q_ASSERT(!m_deviceRemovalObserver);
}

qsizetype QPipewireAudioSinkStream::bytesFree() const
{
    return std::visit([&](auto &ringbuffer) {
        using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;
        return ringbuffer.free() * sizeof(SampleType);
    }, m_ringbuffer);
}

bool QPipewireAudioSinkStream::start(QIODevice *device, ObjectSerial sinkNodeSerial)
{
    m_device = device;
    pullFromQIODevice();

    // consumed from audio thread
    m_ringbufferHasSpaceConnection = QObject::connect(&m_ringbufferHasSpaceEvent,
                                                      &QAutoResetEvent::activated, device, [this] {
        pullFromQIODevice();
    });

    // pushed to device
    m_iodeviceHasNewDataConnection =
            QObject::connect(device, &QIODevice::readyRead, device, [this] {
        pullFromQIODevice();
    });

    assert(hasStream());
    bool connected = connectStream(sinkNodeSerial, SPA_DIRECTION_OUTPUT);
    if (!connected)
        return false;

    // keep instance alive until PW_STREAM_STATE_UNCONNECTED
    m_self = shared_from_this();

    return true;
}

QIODevice *QPipewireAudioSinkStream::start(ObjectSerial sinkNodeSerial)
{
    std::visit([&](auto &rb) {
        using SampleType = typename std::decay_t<decltype(rb)>::ValueType;

        m_ringbufferAdapter = std::make_unique<QIODeviceRingBufferWriter<SampleType>>(&rb);
    }, m_ringbuffer);

    m_ringbufferAdapter->open(QIODevice::WriteOnly | QIODevice::Unbuffered);

    m_streamIsIdle = true;
    bool started = start(m_ringbufferAdapter.get(), sinkNodeSerial);
    if (!started)
        return nullptr;

    QObject::connect(m_ringbufferAdapter.get(), &QIODevice::readyRead, m_parent, [this] {
        m_parent->streamIdle(false);
    });

    return m_ringbufferAdapter.get();
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

    m_stopRequested.store(true, std::memory_order_release);
    m_parent = nullptr;

    // disconnect ringbuffer from QIODevice
    QObject::disconnect(m_ringbufferHasSpaceConnection);
    QObject::disconnect(m_iodeviceHasNewDataConnection);

    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer) {
        // disconnect immediately
        disconnectStream();
    }

    unregisterDeviceObserver();
}

void QPipewireAudioSinkStream::setVolume(qreal volume)
{
    m_volume.store(volume, std::memory_order_relaxed);
}

std::chrono::microseconds QPipewireAudioSinkStream::processedDuration()
{
    uint64_t totalNumberOfFrames =
            m_totalNumberOfSamplesConsumedFromRingbuffer.load(std::memory_order_relaxed)
            / m_format.channelCount();

    return std::chrono::microseconds{
        m_format.durationForFrames(totalNumberOfFrames),
    };
}

void QPipewireAudioSinkStream::handleDeviceRemoved()
{
    if (!m_stopRequested)
        // note: as long as the stream is not stopped, m_parent is valid
        m_parent->updateError(QAudio::Error::IOError);
}

void QPipewireAudioSinkStream::prepareFormat(const QAudioFormat &format,
                                             std::optional<qsizetype> ringbufferSize)
{
    // prepare ringbuffer
    int numberOfFrames = format.sampleRate() / 4; // 250ms
    int bufferSize = ringbufferSize.value_or(format.channelCount() * numberOfFrames);

    switch (format.sampleFormat()) {
    case SampleFormat::Float:
        m_ringbuffer.emplace<QAudioRingBuffer<float>>(bufferSize);
        break;
    case SampleFormat::Int16:
        m_ringbuffer.emplace<QAudioRingBuffer<int16_t>>(bufferSize);
        break;
    case SampleFormat::Int32:
        m_ringbuffer.emplace<QAudioRingBuffer<int32_t>>(bufferSize);
        break;
    case SampleFormat::UInt8:
        m_ringbuffer.emplace<QAudioRingBuffer<uint8_t>>(bufferSize);
        break;

    default:
        qFatal() << "invalid sample format";
    }

    m_strideBytes = format.bytesPerSample() * format.channelCount();
}

void QPipewireAudioSinkStream::process() QT_MM_NONBLOCKING
{
    struct pw_buffer *b = pw_stream_dequeue_buffer(m_stream.get());
    if (!b) {
        qCritical() << "pw_stream_dequeue_buffer failed";
        return;
    }

    struct spa_buffer *buf = b->buffer;
    uint64_t numberOfFrames = buf->datas[0].maxsize / m_strideBytes;

#if PW_CHECK_VERSION(0, 3, 49)
    if (pw_check_library_version(0, 3, 49))
        // LATER: drop support for 0.3.49
        if (b->requested)
            numberOfFrames = std::min(b->requested, numberOfFrames);
#endif

    const uint64_t requestedSamples = numberOfFrames * m_format.channelCount();

    QSpan<std::byte> writeBuffer{
        reinterpret_cast<std::byte *>(buf->datas[0].data),
        qsizetype(requestedSamples * m_format.bytesPerSample()),
    };

    bool stopRequested = m_stopRequested.load(std::memory_order_acquire);
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

    performXRunDetection(numberOfFrames);

    qreal volume = m_volume.load(std::memory_order_relaxed);
    uint64_t samplesWritten = std::visit([&](auto &ringbuffer) {
        uint64_t written = ringbuffer.consume(requestedSamples, [&](auto bufferRegion) {
            QAudioHelperInternal::applyVolume(volume, m_format, as_bytes(bufferRegion),
                                              take(writeBuffer, bufferRegion.size_bytes()));
            writeBuffer = drop(writeBuffer, bufferRegion.size_bytes());
        });

        if (notificationThreshold == 0 || ringbuffer.free() > notificationThreshold)
            m_ringbufferHasSpaceEvent.set();

        return written;
    }, m_ringbuffer);

    if (samplesWritten == requestedSamples)
        Q_ASSERT(samplesWritten / m_format.channelCount() == numberOfFrames);

    m_totalNumberOfSamplesConsumedFromRingbuffer += samplesWritten < requestedSamples
            ? samplesWritten / m_format.channelCount()
            : numberOfFrames;

    if (!stopRequested) {
        performIdleDetection(samplesWritten, requestedSamples);

        if (samplesWritten < requestedSamples) {
            // ringbuffer empty, we fill the rest with zero
            std::fill(writeBuffer.begin(), writeBuffer.end(), std::byte{ 0 });
            uint64_t zeroSamples = writeBuffer.size() / m_format.bytesPerSample();
            samplesWritten += zeroSamples;
        }
    } else {
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

void QPipewireAudioSinkStream::pullFromQIODevice()
{
    if (!m_device)
        return;

    while (true) {
        bool allReadsDone = std::visit([&](auto &ringbuffer) {
            using SampleType = typename std::decay_t<decltype(ringbuffer)>::ValueType;

            static constexpr uint64_t sizeMask = ~(uint64_t(sizeof(SampleType)) - 1);

            qint64 bytesAvailableInDevice = m_device->bytesAvailable() & sizeMask;
            if (!bytesAvailableInDevice)
                return true; // no data in iodevice

            qint64 samplesAvailableInDevice = bytesAvailableInDevice / sizeof(SampleType);

            auto writeRegion = ringbuffer.acquireWriteRegion(samplesAvailableInDevice);
            if (writeRegion.empty())
                return true; // no space in ringbuffer

            auto writeRegionBytes = as_writable_bytes(writeRegion);

            qint64 bytesRead = m_device->read(reinterpret_cast<char *>(writeRegionBytes.data()),
                                              writeRegionBytes.size());
            Q_ASSERT(bytesRead == writeRegionBytes.size());

            ringbuffer.releaseWriteRegion(bytesRead / sizeof(SampleType));

            return false;
        }, m_ringbuffer);

        if (allReadsDone)
            return;

        m_parent->streamIdle(false);
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
    buf->datas[0].chunk->stride = m_strideBytes;
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

void QPipewireAudioSinkStream::performIdleDetection(uint64_t samplesWritten, uint64_t sampleCount)
{
    bool streamWasIdle = m_streamIsIdle.load(std::memory_order_relaxed);

    if (streamWasIdle && samplesWritten > 0) {
        if constexpr (pipewireRealtimeTracing)
            qCDebug(lcPipewireAudioSink) << "QPipewireAudioSinkStream not idle anymore";
        m_streamIsIdle = false;
        m_streamIdleDetectionNotifier.set();
    }
    if (!streamWasIdle && samplesWritten < sampleCount) {
        if constexpr (pipewireRealtimeTracing)
            qCDebug(lcPipewireAudioSink) << "QPipewireAudioSinkStream is idle";
        m_streamIsIdle = true;
        m_streamIdleDetectionNotifier.set();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QPipewireAudioSink

QPipewireAudioSink::QPipewireAudioSink(const QAudioDevice &device, const QAudioFormat &format, QObject *parent)
    : BaseClass(device, format, parent)
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
        updateError(QtAudio::Error::OpenError);
        return;
    }

    std::optional<ObjectSerial> deviceSerial = findSinkNodeSerial();
    if (!deviceSerial) {
        qWarning() << "Cannot find device: " << privateDevice()->nodeName();
        updateError(QtAudio::Error::OpenError);
        return;
    }

    m_stream = std::make_shared<QPipewireAudioSinkStream>(this, format(), m_bufferSize,
                                                          m_hardwareBufferSize);
    if (!m_stream->hasStream()) {
        updateError(QtAudio::Error::OpenError);
        return;
    }

    m_stream->setVolume(m_volume);

    bool started = starter(m_stream, *deviceSerial);
    if (started)
        setUserOwnedState(QtAudio::State::ActiveState);
    else
        updateError(QtAudio::Error::OpenError);
}

using SharedSinkStream = std::shared_ptr<QPipewireAudioSinkStream>;

void QPipewireAudioSink::start(QIODevice *device)
{
    startHelper([&](const SharedSinkStream &stream, ObjectSerial deviceSerial) {
        return stream->start(device, deviceSerial);
    });
}

QIODevice *QPipewireAudioSink::start()
{
    QIODevice *deviceToReturn{};

    startHelper([&](const SharedSinkStream &stream, ObjectSerial deviceSerial) {
        deviceToReturn = stream->start(deviceSerial);
        m_streamIsIdle = true;
        return bool(deviceToReturn);
    });

    return deviceToReturn;
}

void QPipewireAudioSink::stop()
{
    if (!m_stream)
        return;

    m_stream->stop(ShutdownPolicy::DrainRingbuffer);
    setUserOwnedState(QtAudio::State::StoppedState);
    m_stream.reset();
}

void QPipewireAudioSink::reset()
{
    if (!m_stream)
        return;

    m_stream->stop(ShutdownPolicy::DiscardRingbuffer);
    setUserOwnedState(QtAudio::State::StoppedState);
    m_stream.reset();
}

void QPipewireAudioSink::suspend()
{
    if (!m_stream)
        return;

    m_stream->suspend();

    setUserOwnedState(QtAudio::State::SuspendedState);
}

void QPipewireAudioSink::resume()
{
    if (!m_stream)
        return;

    if (m_userOwnedState == QtAudio::State::ActiveState)
        return;

    m_stream->resume();

    setUserOwnedState(QtAudio::State::ActiveState);
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

std::optional<ObjectSerial> QPipewireAudioSink::findSinkNodeSerial()
{
    QByteArray nodeName = privateDevice()->nodeName();

    return QAudioContextManager::deviceMonitor().findSinkNodeSerial(std::string_view{
            nodeName.data(),
            size_t(nodeName.size()),
    });
}

} // namespace QtPipeWire

QT_END_NAMESPACE
